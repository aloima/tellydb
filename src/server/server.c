#include "../../headers/server.h"
#include "../../headers/database.h"
#include "../../headers/commands.h"
#include "../../headers/utils.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>

static int sockfd;
static SSL_CTX *ctx;
static struct pollfd *fds;
static uint32_t nfds;
static struct Configuration *conf;
static time_t start_at;
static uint64_t age;

struct Configuration *get_server_configuration() {
  return conf;
}

void get_server_time(time_t *server_start_at, uint64_t *server_age) {
  *server_start_at = start_at;
  *server_age = age;
}

static int setnonblocking(int sockfd) {
  if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1) return -1;
  return 0;
}

void terminate_connection(const int connfd) {
  const struct Client *client = get_client(connfd);
  write_log(LOG_INFO, "Client #%d is disconnected.", client->id);

  for (uint32_t i = 1; i < nfds; ++i) {
    if (fds[i].fd == connfd) {
      memcpy(fds + i, fds + i + 1, (nfds - i - 1) * sizeof(struct pollfd));
      break;
    }
  }

  nfds -= 1;
  fds = realloc(fds, nfds * sizeof(struct pollfd));

  if (conf->tls) SSL_shutdown(client->ssl);
  close(connfd);
  remove_client(connfd);
}

static void close_server() {
  struct Client **clients = get_clients();
  const uint32_t client_count = get_client_count();

  for (uint32_t i = 0; i < client_count; ++i) {
    const struct Client *client = clients[0];
    write_log(LOG_INFO, "Client #%d is terminated.", client->id);

    close(client->connfd);
    remove_client(client->connfd);
  }

  const uint64_t server_age = age + difftime(time(NULL), start_at);
  clock_t start = clock();
  save_data(server_age);
  close_database_fd();
  write_log(LOG_INFO, "Saved data and closed database file in %.2f seconds.", ((float) clock() - start) / CLOCKS_PER_SEC);

  deactive_transaction_thread();
  usleep(15);
  write_log(LOG_INFO, "Exited transaction thread.");

  free_kdf();
  free_constant_passwords();
  free_passwords();
  free_transactions();
  free_commands();
  free_cache();
  if (conf->tls) SSL_CTX_free(ctx);
  close(sockfd);
  free(fds);
  write_log(LOG_INFO, "Free'd all memory blocks and closed the server.");

  write_log(LOG_INFO, "Closing log file, free'ing configuration and exiting the process...");
  save_and_close_logs();
  free_configuration(conf);

  unlink(".tellylock");
  exit(EXIT_SUCCESS);
}

static void sigint_signal(__attribute__((unused)) int arg) {
  write_log(LOG_WARN, "Received SIGINT signal, closing the server...", LOG_WARN);
  close_server();
}

void start_server(struct Configuration *config) {
  conf = config;
  struct stat lock;

  if (stat(".tellylock", &lock) != -1) {
    write_log(LOG_ERR, "tellydb is already opened in this directory.");
    free_configuration(config);
    exit(EXIT_FAILURE);
  } else creat(".tellylock", 0);

  if (!initialize_logs(conf)) {
    write_log(LOG_ERR, "Cannot initialized logs.");
    write_log(LOG_INFO, "Initialized configuration.");
  } else {
    write_log(LOG_INFO, "Initialized logs and configuration.");
  }

  write_log(LOG_INFO, "version=" VERSION ", commit hash=" GIT_HASH);

  if (conf->default_conf) {
    write_log(LOG_WARN, (
      "There is no configuration file, using default configuration. "
      "To specify, create .tellyconf file or use `telly config /path/to/file`."
    ));
  }

  if (conf->tls) {
    ctx = SSL_CTX_new(TLS_server_method());

    if (!ctx) {
      write_log(LOG_ERR, "Cannot open SSL context, safely exiting...");
      free_configuration(conf);
      unlink(".tellylock");
      return;
    }

    if (SSL_CTX_use_certificate_file(ctx, conf->cert, SSL_FILETYPE_PEM) <= 0) {
      write_log(LOG_ERR, "Server certificate file cannot be used.");
      SSL_CTX_free(ctx);
      free_configuration(conf);
      unlink(".tellylock");
      return;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, conf->private_key, SSL_FILETYPE_PEM) <= 0 ) {
      write_log(LOG_ERR, "Server private key cannot be used.");
      SSL_CTX_free(ctx);
      free_configuration(conf);
      unlink(".tellylock");
      return;
    }

    write_log(LOG_INFO, "Created SSL context.");
  }

  load_commands();
  write_log(LOG_INFO, "Initialized commands.");

  create_transaction_thread(config);
  write_log(LOG_INFO, "Created transaction thread.");

  signal(SIGINT, sigint_signal);

  struct sockaddr_in servaddr;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    deactive_transaction_thread();
    usleep(15);
    free_commands();
    write_log(LOG_ERR, "Cannot open socket, safely exiting...");
    free_configuration(conf);
    unlink(".tellylock");
    return;
  }

  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(conf->port);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) != 0) {
    deactive_transaction_thread();
    usleep(15);
    free_commands();
    close(sockfd);
    write_log(LOG_ERR, "Cannot bind socket and address, safely exiting...");
    free_configuration(conf);
    unlink(".tellylock");
    return;
  }

  if (setnonblocking(sockfd) == -1) {
    deactive_transaction_thread();
    usleep(15);
    free_commands();
    close(sockfd);
    write_log(LOG_ERR, "Cannot set non-blocking socket, safely exiting...");
    free_configuration(conf);
    unlink(".tellylock");
    return;
  }

  if (listen(sockfd, 10) != 0) { 
    deactive_transaction_thread();
    usleep(15);
    free_commands();
    close(sockfd);
    write_log(LOG_ERR, "Cannot listen socket, safely exiting...");
    free_configuration(conf);
    unlink(".tellylock");
    return;
  }

  if (!create_constant_passwords()) {
    deactive_transaction_thread();
    usleep(15);
    free_constant_passwords();
    free_commands();
    close(sockfd);
    write_log(LOG_ERR, "Safely exiting...");
    free_configuration(conf);
    unlink(".tellylock");
    return;
  }

  initialize_kdf();
  write_log(LOG_INFO, "Created constant passwords and key deriving algorithm.");

  if (create_cache() != NULL) {
    write_log(LOG_INFO, "Created cache.");
  } else {
    deactive_transaction_thread();
    usleep(15);
    free_kdf();
    free_constant_passwords();
    free_commands();
    close(sockfd);
    write_log(LOG_ERR, "Safely exiting...");
    free_configuration(conf);
    unlink(".tellylock");
    return;
  }

  if (!open_database_fd(conf->data_file, &age)) {
    deactive_transaction_thread();
    usleep(15);
    free_kdf();
    free_constant_passwords();
    free_commands();
    free_cache();
    close(sockfd);
    write_log(LOG_WARN, "Safely exiting...");
    free_configuration(conf);
    unlink(".tellylock");
    return;
  }

  write_log(LOG_INFO, "tellydb server age: %ld seconds", age);

  nfds = 1;
  fds = malloc(sizeof(struct pollfd));
  fds[0].fd = sockfd;
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  start_at = time(NULL);
  write_log(LOG_INFO, "Server is listening on %d port for accepting connections...", conf->port);

  while (true) {
    const int ret = poll(fds, nfds, -1);

    if (ret != -1) {
      for (uint32_t i = 0; i < nfds; ++i) {
        const struct pollfd fd = fds[i];

        if (fd.fd == sockfd && fd.revents & POLLIN) {
          if (conf->max_clients == get_client_count()) {
            write_log(LOG_WARN, "A connection request is rejected, because connected client count of the server is maximum.");
          } else if (UINT32_MAX == get_last_connection_client_id()) {
            write_log(LOG_WARN, "A connection request is rejected, because client that has highest ID number is maximum, so cannot be increased it.");
          } else {
            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);

            const int connfd = accept(sockfd, (struct sockaddr *) &addr, &addr_len);
            struct Client *client = add_client(connfd);

            nfds += 1;
            fds = realloc(fds, nfds * sizeof(struct pollfd));

            const uint32_t at = nfds - 1;
            fds[at].fd = connfd;
            fds[at].events = POLLIN;
            fds[at].revents = 0;

            if (conf->tls) {
              client->ssl = SSL_new(ctx);
              SSL_set_fd(client->ssl, client->connfd);

              if (SSL_accept(client->ssl) <= 0) {
                write_log(LOG_WARN, ("Client #%d cannot be accepted as a SSL client. "
                  "It may try connecting without client authority file, so it is terminating..."), client->id);

                terminate_connection(client->connfd);
                continue;
              }
            }

            write_log(LOG_INFO, "Client #%d is connected.", client->id);
          }
        } else if (fd.revents & POLLIN) {
          struct Client *client = get_client(fd.fd);
          commanddata_t *command = get_command_data(client);

          if (command->close) {
            terminate_connection(fd.fd);
            free(command);
          } else {
            struct Client *client = get_client(fd.fd);

            if (!client->locked) {
              struct Command *commands = get_commands();
              const uint32_t command_count = get_command_count();

              char used[command->name.len + 1];
              to_uppercase(command->name.value, used);

              for (uint32_t i = 0; i < command_count; ++i) {
                if (streq(commands[i].name, used)) {
                  client->command = &commands[i];
                  break;
                }
              }

              add_transaction(client, command);
            } else {
              free_command_data(command);
              _write(client, "-Your client is locked, you cannot use any commands until your client is unlocked\r\n", 83);
            }
          }
        } else if (fd.revents & (POLLERR | POLLNVAL | POLLHUP)) {
          terminate_connection(fd.fd);
        }
      }
    }
  }

  close_server();
}
