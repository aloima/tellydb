#include "../../headers/telly.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <asm-generic/socket.h>

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

static int epfd;
static int sockfd;
static SSL_CTX *ctx = NULL;
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

void terminate_connection(const int connfd) {
  const struct Client *client = get_client(connfd);

  if (epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL) == -1) {
    write_log(LOG_ERR, "Cannot remove Client #%d from multiplexing.", client->id);
    return;
  }

  write_log(LOG_INFO, "Client #%d is disconnected.", client->id);
  if (conf->tls) SSL_shutdown(client->ssl);
  close(connfd);
  remove_client(connfd);
}

static void close_server() {
  struct Client *client;

  while ((client = get_first_client())) {
    write_log(LOG_INFO, "Client #%d is terminated.", client->id);

    if (conf->tls) SSL_shutdown(client->ssl);
    close(client->connfd);
    remove_first_client();
  }

  const uint64_t server_age = age + difftime(time(NULL), start_at);
  const clock_t start = clock();
  save_data(server_age);
  close_database_fd();
  write_log(LOG_INFO, "Saved data and closed database file in %.3f seconds.", ((float) clock() - start) / CLOCKS_PER_SEC);

  FREE_CTX_THREAD_CMD_SOCKET_KDF_PASS(ctx, sockfd);
  close(epfd);
  free_passwords();
  free_transactions();
  free_databases();
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

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    FREE_CTX_THREAD_CMD(ctx);
    write_log(LOG_ERR, "Cannot open socket, safely exiting...");
    FREE_CONF_LOGS(conf);
    return;
  }

  if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1) {
    FREE_CTX_THREAD_CMD_SOCKET(ctx, sockfd);
    write_log(LOG_ERR, "Cannot set non-blocking socket, safely exiting...");
    FREE_CONF_LOGS(conf);
    return;
  }

  const int flag = 1;

  if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == -1) {
    FREE_CTX_THREAD_CMD_SOCKET(ctx, sockfd);
    write_log(LOG_ERR, "Cannot set no-delay socket, safely exiting...");
    FREE_CONF_LOGS(conf);
    return;
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) {
    FREE_CTX_THREAD_CMD_SOCKET(ctx, sockfd);
    write_log(LOG_ERR, "Cannot set reusable socket, safely exiting...");
    FREE_CONF_LOGS(conf);
    return;
  }

  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(conf->port);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) != 0) {
    FREE_CTX_THREAD_CMD_SOCKET(ctx, sockfd);
    write_log(LOG_ERR, "Cannot bind socket and address, safely exiting...");
    FREE_CONF_LOGS(conf);
    return;
  }

  if (listen(sockfd, conf->max_clients) != 0) {
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

  if (!open_database_fd(conf, &age)) {
    FREE_CTX_THREAD_CMD_SOCKET_KDF_PASS(ctx, sockfd);
    write_log(LOG_WARN, "Safely exiting...");
    FREE_CONF_LOGS(conf);
    return;
  }

  write_log(LOG_INFO, "tellydb server age: %ld seconds", age);

  if ((epfd = epoll_create1(0)) == -1) {
    FREE_CTX_THREAD_CMD_SOCKET_KDF_PASS(ctx, sockfd);
		write_log(LOG_ERR, "Cannot create epoll instance.");
    FREE_CONF_LOGS(conf);
    return;
  }

  struct epoll_event event, events[32];
  event.events = EPOLLIN;
  event.data.fd = sockfd;

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event) == -1) {
    FREE_CTX_THREAD_CMD_SOCKET_KDF_PASS(ctx, sockfd);
    close(epfd);
		write_log(LOG_ERR, "Cannot add server to epoll instance.");
    FREE_CONF_LOGS(conf);
    return;
	}

  start_at = time(NULL);
  write_log(LOG_INFO, "Server is listening on %d port for accepting connections...", conf->port);

  while (true) {
    const int nfds = epoll_wait(epfd, events, 32, -1);

    for (int i = 0; i < nfds; ++i) {
      const int fd = events[i].data.fd;

      if (fd == sockfd) {
        if (UINT32_MAX == get_last_connection_client_id()) {
          write_log(LOG_WARN, "A connection is rejected, because client that has highest ID number is maximum, so cannot be increased it.");
          continue;
        }

        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);

        const int connfd = accept(sockfd, (struct sockaddr *) &addr, &addr_len);
        struct Client *client = add_client(connfd);

        fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFL, 0) | O_NONBLOCK);

        event.events = (EPOLLIN | EPOLLET);
        event.data.fd = connfd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &event);

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
        struct Client *client = get_client(fd);
        char buf[RESP_BUF_SIZE];
        uint32_t at = 0;
        int32_t size = _read(client, buf, RESP_BUF_SIZE);

        if (size == 0) {
          terminate_connection(fd);
          continue;
        }

        while (true) {
          commanddata_t *data = get_command_data(client, buf, &at, &size);
          struct Client *client = get_client(fd);

          if (!client->locked) {
            struct Command *command = NULL;
            to_uppercase(data->name.value, data->name.value);

            for (uint32_t i = 0; i < command_count; ++i) {
              if (streq(commands[i].name, data->name.value)) {
                command = &commands[i];
                client->command = &commands[i];
                break;
              }
            }

            add_transaction(client, command, data);
          } else {
            free_command_data(data);
            _write(client, "-Your client is locked, you cannot use any commands until your client is unlocked\r\n", 83);
          }

          if (size == (int32_t) at) {
            size = _read(client, buf, RESP_BUF_SIZE);
            at = 0;
            if (size == -1) break;
          }
        }
      }
    }
  }

  close_server();
}
