#include <telly.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/epoll.h>
#elif __APPLE__
#include <sys/event.h>
#include <sys/time.h>
#endif

#include <openssl/ssl.h>
#include <openssl/crypto.h>

static inline int read_client(struct Client *client, char *buf, int32_t *size, int32_t *at) {
  *size = _read(client, buf, RESP_BUF_SIZE);
  *at = 0;

  if (*size == -1) {
    return -1;
  }

  return 0;
}

static inline int accept_client(const int sockfd, struct Configuration *conf, SSL_CTX *ctx, const int eventfd) {
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);

  const int connfd = accept(sockfd, (struct sockaddr *) &addr, &addr_len);
  struct Client *client = add_client(connfd);

  fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFL, 0) | O_NONBLOCK);

  event_t event;
#ifdef __linux__
  event.events = (EPOLLIN | EPOLLET);
  event.data.fd = connfd;
  epoll_ctl(eventfd, EPOLL_CTL_ADD, connfd, &event);
#elif __APPLE__
  EV_SET(&event, connfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
  kevent(eventfd, &event, 1, NULL, 0, NULL);
#endif

  if (conf->tls) {
    client->ssl = SSL_new(ctx);
    SSL_set_fd(client->ssl, client->connfd);

    if (SSL_accept(client->ssl) <= 0) {
      write_log(LOG_WARN, "Cannot accept Client #%" PRIu32 " because of SSL. Please check client authority file.", client->id);
      terminate_connection(client->connfd);
      return -1;
    }
  }

  write_log(LOG_INFO, "Client #%" PRIu32 " is connected.", client->id);
  return 0;
}

static inline void unknown_command(struct Client *client, commandname_t name) {
  char *buf = malloc(name.len + 22);
  const size_t nbytes = sprintf(buf, "-Unknown command '%s'\r\n", name.value);

  _write(client, buf, nbytes);
  free(buf);
}

void handle_events(struct Configuration *conf, SSL_CTX *ctx, const int sockfd, struct Command *commands, const int eventfd) {
  event_t events[32];

  while (true) {
#if defined(__linux__)
    const int nfds = epoll_wait(eventfd, events, 32, -1);

    for (int i = 0; i < nfds; ++i) {
      const int fd = events[i].data.fd;
#elif defined(__APPLE__)
    const int nfds = kevent(kq, NULL, 0, events, 32, NULL);

    for (int i = 0; i < nfds; ++i) {
      const int fd = events[i].ident;
#endif

      if (fd == sockfd) {
        if (UINT32_MAX == get_last_connection_client_id()) {
          write_log(LOG_WARN, "A connection is rejected, because client that has highest ID number is maximum, so cannot be increased it.");
          continue;
        }

        if (accept_client(sockfd, conf, ctx, eventfd) == -1) {
          continue;
        }
      } else {
        struct Client *client = get_client(fd);
        char buf[RESP_BUF_SIZE];
        int32_t at = 0;
        int32_t size = _read(client, buf, RESP_BUF_SIZE);

        if (size == 0) {
          terminate_connection(fd);
          continue;
        }

        while (size != -1) {
          commanddata_t data;

          if (!get_command_data(client, buf, &at, &size, &data)) {
            continue;
          }

          if (size == at) {
            read_client(client, buf, &size, &at);
          }

          if (client->locked) {
            free_command_data(data);
            WRITE_ERROR_MESSAGE(client, "Your client is locked, you cannot use any commands until your client is unlocked");
            continue;
          }

          to_uppercase(data.name.value, data.name.value);

          const struct CommandIndex *command_index = get_command_index(data.name.value, data.name.len);

          if (!command_index) {
            unknown_command(client, data.name);
            continue;
          }

          struct Command command = commands[command_index->idx];
          client->command = &command;

          if (!add_transaction(client, command, data)) {
            free_command_data(data);
            WRITE_ERROR_MESSAGE(client, "Transaction cannot be enqueued because of server settings");
            write_log(LOG_WARN, "Transaction count reached their limit, so next transactions cannot be added.");
            continue;
          }

          if (client->waiting_block && !streq(command.name, "EXEC") && !streq(command.name, "DISCARD") && !streq(command.name, "MULTI")) {
            _write(client, "+QUEUED\r\n", 9);
          }
        }
      }
    }
  }
}
