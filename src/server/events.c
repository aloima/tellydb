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

#if defined(__linux__)
#include <sys/epoll.h>
#elif defined(__APPLE__)
#include <sys/event.h>
#include <sys/time.h>
#endif

#include <openssl/ssl.h>
#include <openssl/crypto.h>

static inline void read_client(struct Client *client, char *buf, int32_t *size, int32_t *at) {
  *size = _read(client, buf, RESP_BUF_SIZE);
  *at = 0;
}

static inline int accept_client(const int sockfd, struct Configuration *conf, SSL_CTX *ctx, const int eventfd) {
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);

  const int connfd = accept(sockfd, (struct sockaddr *) &addr, &addr_len);

  if (connfd == -1) {
    write_log(LOG_WARN, "Cannot accept a connection because of sockets.");
    return -1;
  }

  struct Client *client = add_client(connfd);

  if (!client) {
    terminate_connection(client->connfd);
    return -1;
  }

  if ((fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFL, 0) | O_NONBLOCK)) == -1) {
    write_log(LOG_WARN, "Cannot accept Client #%" PRIu32 ", because cannot set non-blocking file descriptor.", client->id);
    terminate_connection(client->connfd);
    return -1;
  }

  event_t event;
#if defined(__linux__)
  event.events = (EPOLLIN | EPOLLET | EPOLLHUP | EPOLLRDHUP);
  event.data.fd = connfd;

  if ((epoll_ctl(eventfd, EPOLL_CTL_ADD, connfd, &event)) == -1) {
#elif defined(__APPLE__)
  EV_SET(&event, connfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);

  if ((kevent(eventfd, &event, 1, NULL, 0, NULL)) == -1) {
#endif
    write_log(LOG_WARN, "Cannot accept Client #%" PRIu32 ", because cannot add it to multiplexing queue.", client->id);
    terminate_connection(client->connfd);
    return -1;
  }

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

// TODO: thread-race for transactions, some executions will not be executed for inline commands
void handle_events(struct Configuration *conf, SSL_CTX *ctx, const int sockfd, struct Command *commands, const int eventfd) {
  event_t events[32];
  char buf[RESP_BUF_SIZE];

  while (true) {
#if defined(__linux__)
    const int nfds = epoll_wait(eventfd, events, 32, -1);

    for (int i = 0; i < nfds; ++i) {
      const int fd = events[i].data.fd;
#elif defined(__APPLE__)
    const int nfds = kevent(eventfd, NULL, 0, events, 32, NULL);

    for (int i = 0; i < nfds; ++i) {
      const int fd = events[i].ident;
#endif

      if (fd == sockfd) {
        if (UINT32_MAX == get_last_connection_client_id()) {
          write_log(LOG_WARN, "A connection is rejected, because client that has highest ID number is maximum.");
          continue;
        }

        // If client cannot be accepted, it continues already. No need condition.
        accept_client(sockfd, conf, ctx, eventfd);
        continue;
      }

      struct Client *client = get_client(fd);
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
          if (size != RESP_BUF_SIZE) {
            size = -1;
          } else {
            read_client(client, buf, &size, &at);
          }
        }

        if (client->locked) {
          free_command_data(data);
          WRITE_ERROR_MESSAGE(client, "Your client is locked, you cannot use any commands until your client is unlocked");
          continue;
        }

        to_uppercase(CREATE_STRING(data.name.value, data.name.len), data.name.value);
        const struct CommandIndex *command_index = get_command_index(data.name.value, data.name.len);

        if (!command_index) {
          unknown_command(client, data.name);
          continue;
        }

        struct Command command = commands[command_index->idx];
        client->command = &command;

        if (!add_transaction(client, command_index->idx, data)) {
          free_command_data(data);
          WRITE_ERROR_MESSAGE(client, "Transaction cannot be enqueued because of server settings");
          write_log(LOG_WARN, "Transaction count reached their limit, so next transactions cannot be added.");
          continue;
        }

        if (client->waiting_block && !streq(command.name, "EXEC") && !streq(command.name, "DISCARD") && !streq(command.name, "MULTI")) {
          _write(client, "+QUEUED\r\n", 9);
        }
      }

#if defined(__linux__)
      if (fd != sockfd && (events[i].events & (EPOLLRDHUP | EPOLLHUP))) {
#elif defined(__APPLE__)
      if (fd != sockfd && (events[i].flags & EV_EOF)) {
#endif
        terminate_connection(fd);
        continue;
      }
    }
  }
}
