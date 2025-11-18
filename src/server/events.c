#include <telly.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

#if defined(__linux__)
  #include <sys/epoll.h>

  #define GET_EVENT_FD(event) (event).data.fd
  #define WAIT_EVENTS(eventfd, events, count) epoll_wait((eventfd), (events), (count), -1)
  #define GET_EVENT_DATA(event) (event).data.ptr
  #define IS_CONNECTION_CLOSED(event) ((event).events & (EPOLLRDHUP | EPOLLHUP))
  #define ADD_TO_MULTIPLEXING(eventfd, connfd, event) epoll_ctl((eventfd), EPOLL_CTL_ADD, (connfd), &(event))

  #define PREPARE_EVENT(event, client, connfd) do { \
    (void) connfd; \
    (event).events = (EPOLLIN | EPOLLET | EPOLLHUP | EPOLLRDHUP); \
    (event).data.ptr = (client); \
  } while (0)
#elif defined(__APPLE__)
  #include <sys/event.h>
  #include <sys/time.h>
  #include <fcntl.h>

  #define GET_EVENT_FD(event) (event).ident
  #define WAIT_EVENTS(eventfd, events, count) kevent((eventfd), NULL, 0, (events), (count), NULL)
  #define GET_EVENT_DATA(event) (event).udata
  #define IS_CONNECTION_CLOSED(event) ((event).flags & EV_EOF)

  #define ADD_TO_MULTIPLEXING(eventfd, connfd, event) kevent((eventfd), &(event), 1, NULL, 0, NULL)

  #define PREPARE_EVENT(event, client, connfd) \
    EV_SET(&(event), (connfd), EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, (client))
#endif

#include <openssl/ssl.h>
#include <openssl/crypto.h>

static inline int accept_client(struct Server *server) {
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);

#if defined(__linux__)
  const int connfd = accept4(server->sockfd, (struct sockaddr *) &addr, &addr_len, SOCK_NONBLOCK);

  if (connfd == -1) {
    write_log(LOG_WARN, "Cannot accept a connection as non-blocking because of sockets.");
    return -1;
  }
#elif defined(__APPLE__)
  const int connfd = accept(server->sockfd, (struct sockaddr *) &addr, &addr_len);

  if (connfd == -1) {
    write_log(LOG_WARN, "Cannot accept a connection because of sockets.");
    return -1;
  }

  if ((fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFL, 0) | O_NONBLOCK)) == -1) {
    write_log(LOG_WARN, "Cannot accept a connection, because cannot set as non-blocking file descriptor.");
    close(connfd);
    return -1;
  }
#endif

  {
    const int flag = 1;

    if (setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == -1) {
      write_log(LOG_WARN, "Cannot accept a connection, because cannot set as non-delaying file descriptor.");
      close(connfd);
      return -1;
    }
  }

  if (get_client_count() == server->conf->max_clients) {
    write(connfd, "-Server is reached maximum client limit\r\n", 14);
    close(connfd);
    return -1;
  }

  struct Client *client = add_client(connfd);

  event_t event;
  PREPARE_EVENT(event, client, connfd);

  if (ADD_TO_MULTIPLEXING(server->eventfd, connfd, event) == -1) {
    write_log(LOG_WARN, "Cannot accept Client #%" PRIu32 ", because cannot add it to multiplexing queue.", client->id);
    terminate_connection(client);
    return -1;
  }

  if (server->conf->tls) {
    client->ssl = SSL_new(server->ctx);
    SSL_set_fd(client->ssl, client->connfd);

    if (SSL_accept(client->ssl) <= 0) {
      write_log(LOG_WARN, "Cannot accept Client #%" PRIu32 " because of SSL. Please check client authority file.", client->id);
      terminate_connection(client);
      return -1;
    }
  }

  write_log(LOG_DBG, "Client #%" PRIu32 " is connected.", client->id);
  return 0;
}

static inline void unknown_command(struct Client *client, commandname_t name) {
  char *buf = malloc(name.len + 22);
  const size_t nbytes = sprintf(buf, "-Unknown command '%s'\r\n", name.value);

  _write(client, buf, nbytes);
  free(buf);
}

// TODO: thread-race for transactions, some executions will not be executed for inline commands
void handle_events(struct Server *server) {
  event_t events[512];
  char buf[RESP_BUF_SIZE];

  const int sockfd = server->sockfd;
  const int eventfd = server->eventfd;
  struct Command *commands = server->commands;

  while (!server->closed) {
    const int nfds = WAIT_EVENTS(eventfd, events, 512);

    for (int i = 0; i < nfds; ++i) {
      const int fd = GET_EVENT_FD(events[i]);

      if (fd == sockfd) {
        if (UINT32_MAX == get_last_connection_client_id()) {
          write_log(LOG_WARN, "A connection is rejected, because client that has highest ID number is maximum.");
          continue;
        }

        // If client cannot be accepted, it continues already. No need condition.
        accept_client(server);
        continue;
      }

      struct Client *client = GET_EVENT_DATA(events[i]);

      int32_t at = 0;
      int32_t size = _read(client, buf, RESP_BUF_SIZE);

      if (size == 0) {
        terminate_connection(client);
        continue;
      }

      while (size != -1) {
        commanddata_t data;
        if (!get_command_data(client, buf, &at, &size, &data)) continue;

        if (size == at) {
          if (size != RESP_BUF_SIZE) {
            size = -1;
          } else {
            size = _read(client, buf, RESP_BUF_SIZE);
            at = 0;
          }
        }

        if (client->locked) {
          free_command_data(data);
          WRITE_ERROR_MESSAGE(client, "Your client is locked, you cannot use any commands until your client is unlocked");
          continue;
        }

        const struct CommandIndex *command_index = get_command_index(data.name.value, data.name.len);

        if (!command_index) {
          unknown_command(client, data.name);
          continue;
        }

        const uint64_t command_idx = command_index->idx;
        client->command = &commands[command_idx];

        if (!add_transaction(client, command_idx, &data)) {
          free_command_data(data);
          WRITE_ERROR_MESSAGE(client, "Transaction cannot be enqueued because of server settings");
          write_log(LOG_WARN, "Transaction count reached their limit, so next transactions cannot be added.");
          continue;
        }

        if (client->waiting_block && !IS_RELATED_TO_WAITING_TX(command_idx)) _write(client, "+QUEUED\r\n", 9);
      }

      if (fd != sockfd && IS_CONNECTION_CLOSED(events[i])) {
        terminate_connection(client);
        continue;
      }
    }
  }
}
