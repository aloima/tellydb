#include "../../headers/telly.h"

#include <string.h>
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

#include <openssl/ssl.h>
#include <openssl/crypto.h>

#define FREE_CONF_LOGS(conf) {\
  save_and_close_logs();\
  free_configuration(conf);\
}

#define FREE_CTX_THREAD_CMD(ctx) {\
  if (ctx) SSL_CTX_free(ctx);\
  deactive_transaction_thread();\
  usleep(15);\
  free_commands();\
}

#define FREE_CTX_THREAD_CMD_SOCKET(ctx, sockfd) {\
  FREE_CTX_THREAD_CMD(ctx);\
  close(sockfd);\
}

#define FREE_CTX_THREAD_CMD_SOCKET_KDF_PASS(ctx, sockfd) {\
  FREE_CTX_THREAD_CMD_SOCKET(ctx, sockfd);\
  free_kdf();\
  free_constant_passwords();\
}

static int sockfd;
static SSL_CTX *ctx = NULL;
static struct pollfd *fds;
static uint32_t nfds;
static struct Configuration *conf;
static time_t start_at;
static uint64_t age;

struct Command *commands;
uint32_t command_count;

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
  struct Client *client;

  while ((client = get_first_client())) {
    write_log(LOG_INFO, "Client #%d is terminated.", client->id);

    close(client->connfd);
    remove_first_client();
  }

  const uint64_t server_age = age + difftime(time(NULL), start_at);
  clock_t start = clock();
  save_data(server_age);
  close_database_fd();
  write_log(LOG_INFO, "Saved data and closed database file in %.2f seconds.", ((float) clock() - start) / CLOCKS_PER_SEC);

  FREE_CTX_THREAD_CMD_SOCKET_KDF_PASS(ctx, sockfd);
  free_passwords();
  free_transactions();
  free_cache();
  free(fds);
  write_log(LOG_INFO, "Free'd all memory blocks and closed the server.");

  write_log(LOG_INFO, "Closing log file, free'ing configuration and exiting the process...");
  FREE_CONF_LOGS(conf);

  exit(EXIT_SUCCESS);
}

static void sigint_signal(__attribute__((unused)) int arg) {
  write_log(LOG_WARN, "Received SIGINT signal, closing the server...", LOG_WARN);
  close_server();
}

void start_server(struct Configuration *config) {
  conf = config;

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
      FREE_CONF_LOGS(conf);
      return;
    }

    if (SSL_CTX_use_certificate_file(ctx, conf->cert, SSL_FILETYPE_PEM) <= 0) {
      write_log(LOG_ERR, "Server certificate file cannot be used.");
      SSL_CTX_free(ctx);
      FREE_CONF_LOGS(conf);
      return;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, conf->private_key, SSL_FILETYPE_PEM) <= 0 ) {
      write_log(LOG_ERR, "Server private key cannot be used.");
      SSL_CTX_free(ctx);
      FREE_CONF_LOGS(conf);
      return;
    }

    write_log(LOG_INFO, "Created SSL context.");
  }

  commands = load_commands();
  command_count = get_command_count();
  write_log(LOG_INFO, "Initialized commands.");

  create_transaction_thread(config);
  write_log(LOG_INFO, "Created transaction thread.");

  signal(SIGINT, sigint_signal);

  struct sockaddr_in servaddr;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    FREE_CTX_THREAD_CMD(ctx);
    write_log(LOG_ERR, "Cannot open socket, safely exiting...");
    FREE_CONF_LOGS(conf);
    return;
  }

  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(conf->port);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) != 0) {
    FREE_CTX_THREAD_CMD_SOCKET(ctx, sockfd);
    write_log(LOG_ERR, "Cannot bind socket and address, safely exiting...");
    FREE_CONF_LOGS(conf);
    return;
  }

  if (setnonblocking(sockfd) == -1) {
    FREE_CTX_THREAD_CMD_SOCKET(ctx, sockfd);
    write_log(LOG_ERR, "Cannot set non-blocking socket, safely exiting...");
    FREE_CONF_LOGS(conf);
    return;
  }

  if (listen(sockfd, 10) != 0) { 
    FREE_CTX_THREAD_CMD_SOCKET(ctx, sockfd);
    write_log(LOG_ERR, "Cannot listen socket, safely exiting...");
    FREE_CONF_LOGS(conf);
    return;
  }

  if (!create_constant_passwords()) {
    FREE_CTX_THREAD_CMD_SOCKET(ctx, sockfd);
    write_log(LOG_ERR, "Safely exiting...");
    FREE_CONF_LOGS(conf);
    return;
  }

  initialize_kdf();
  write_log(LOG_INFO, "Created constant passwords and key deriving algorithm.");

  if (create_cache() != NULL) {
    write_log(LOG_INFO, "Created cache.");
  } else {
    FREE_CTX_THREAD_CMD_SOCKET_KDF_PASS(ctx, sockfd);
    write_log(LOG_ERR, "Safely exiting...");
    FREE_CONF_LOGS(conf);
    return;
  }

  if (!open_database_fd(conf->data_file, &age)) {
    FREE_CTX_THREAD_CMD_SOCKET_KDF_PASS(ctx, sockfd);
    free_cache();
    write_log(LOG_WARN, "Safely exiting...");
    FREE_CONF_LOGS(conf);
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

        if (fd.revents & POLLIN) {
          if (fd.fd == sockfd) {
            if (conf->max_clients == get_client_count()) {
              write_log(LOG_WARN, "A connection is rejected, because connected client count of the server is maximum.");
              continue;
            } else if (UINT32_MAX == get_last_connection_client_id()) {
              write_log(LOG_WARN, "A connection is rejected, because client that has highest ID number is maximum, so cannot be increased it.");
              continue;
            }

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
          } else {
            struct Client *client = get_client(fd.fd);
            commanddata_t *command = get_command_data(client);

            if (command->close) {
              terminate_connection(fd.fd);
              free(command);
            } else {
              struct Client *client = get_client(fd.fd);

              if (!client->locked) {
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
          }
        } else if (fd.revents & (POLLERR | POLLNVAL | POLLHUP)) {
          terminate_connection(fd.fd);
        }
      }
    }
  }

  close_server();
}
