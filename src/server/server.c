#include "../../headers/telly.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>

#include <pthread.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

static uint32_t max_client_id_len;

static pthread_t thread;
static int sockfd;
static struct pollfd *fds = NULL;
static uint32_t nfds;
static struct Configuration *conf;

static int setnonblocking(int sockfd) {
  if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1) return -1;
  return 0;
}

void terminate_connection(const int connfd, struct Configuration *conf) {
  struct Client *client = get_client(connfd);
  char message[26 + max_client_id_len];
  sprintf(message, "Client #%d is disconnected.", client->id);
  write_log(message, LOG_INFO, conf->allowed_log_levels);

  for (uint32_t i = 1; i < nfds; ++i) {
    if (fds[i].fd == connfd) {
      const uint32_t bound = nfds - 1;

      for (uint32_t j = i; j < bound; ++j) {
        memcpy(&fds[i], &fds[i + 1], sizeof(struct pollfd));
      }
    }
  }

  nfds -= 1;
  fds = realloc(fds, nfds * sizeof(struct pollfd));

  close(connfd);
  remove_client(connfd);
}

void close_server() {
  struct Client **clients = get_clients();
  const uint32_t client_count = get_client_count();
  char message[24 + max_client_id_len];

  if (client_count != (nfds - 1)) {
    write_log("Connected client count do not match polling client count", LOG_ERR, conf->allowed_log_levels);
    exit(EXIT_FAILURE);
  } else {
    for (uint32_t i = 0; i < client_count; ++i) {
      struct Client *client = clients[0];
      sprintf(message, "Client #%d is terminated.", client->id);
      write_log(message, LOG_INFO, conf->allowed_log_levels);

      close(client->connfd);
      remove_client(client->connfd);
    }

    write_log("Saving data...", LOG_WARN, conf->allowed_log_levels);
    save_data();
    close_database_file();
    write_log("Saved data and closed database file.", LOG_INFO, conf->allowed_log_levels);

    pthread_cancel(thread);
    pthread_kill(thread, SIGINT);
    free_transactions();
    free_commands();
    close(sockfd);
    free(conf);
    free(fds);

    exit(EXIT_SUCCESS);
  }
}

static void sigint_signal([[maybe_unused]] int arg) {
  write_log("Received SIGINT signal, closing the server...", LOG_WARN, conf->allowed_log_levels);
  close_server();
}

void start_server(struct Configuration *config) {
  load_commands();
  conf = config;
  thread = create_transaction_thread(config);
  signal(SIGINT, sigint_signal);

  struct sockaddr_in servaddr;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    write_log("cannot open socket", LOG_ERR, conf->allowed_log_levels);
    pthread_cancel(thread);
    free_commands();
    return;
  }

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(conf->port);

  if ((bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) != 0) { 
    write_log("cannot bind socket and address", LOG_ERR, conf->allowed_log_levels);
    pthread_cancel(thread);
    free_commands();
    return;
  }

  if (setnonblocking(sockfd) == -1) {
    write_log("cannot set non-blocking socket", LOG_ERR, conf->allowed_log_levels);
    pthread_cancel(thread);
    free_commands();
    return;
  }

  if (listen(sockfd, 10) != 0) { 
    write_log("cannot listen socket", LOG_ERR, conf->allowed_log_levels);
    pthread_cancel(thread);
    free_commands();
    return;
  }

  write_log("Creating cache and opening database file...", LOG_INFO, conf->allowed_log_levels);
  create_cache();
  open_database_file(conf->data_file);
  write_log("Created cache and opened database file.", LOG_INFO, conf->allowed_log_levels);

  nfds = 1;
  fds = malloc(sizeof(struct pollfd));
  fds[0].fd = sockfd;
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  write_log("Server is ready for accepting connections...", LOG_INFO, conf->allowed_log_levels);

  max_client_id_len = get_digit_count(conf->max_clients);

  while (true) {
    int ret = poll(fds, nfds, -1);

    if (ret != -1) {
      for (uint32_t i = 0; i < nfds; ++i) {
        struct pollfd fd = fds[i];

        if (fd.fd == sockfd && fd.revents & POLLIN) {
          if (conf->max_clients == get_client_count()) {
            char message[] = "A connection request is rejected, because connected client count to the server is maximum.";
            write_log(message, LOG_WARN, conf->allowed_log_levels);
          } else {
            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);

            const int connfd = accept(sockfd, (struct sockaddr *) &addr, &addr_len);
            struct Client *client = add_client(connfd, conf->max_clients);

            nfds += 1;
            fds = realloc(fds, nfds * sizeof(struct pollfd));

            const uint32_t at = nfds - 1;
            fds[at].fd = connfd;
            fds[at].events = POLLIN | POLLOUT;
            fds[at].revents = 0;

            char message[23 + max_client_id_len];
            sprintf(message, "Client #%d is connected.", client->id);
            write_log(message, LOG_INFO, conf->allowed_log_levels);
          }
        } else if (fd.revents & POLLIN) {
          respdata_t *data = get_resp_data(fd.fd);

          if (data->type == RDT_CLOSE) {
            terminate_connection(fd.fd, conf);
          } else {
            struct Client *client = get_client(fd.fd);
            struct Command *commands = get_commands();
            const uint32_t command_count = get_command_count();

            const char *used = data->value.array[0]->value.string.value;

            for (uint32_t i = 0; i < command_count; ++i) {
              if (streq(commands[i].name, used)) {
                client->command = &commands[i];
                break;
              }
            }

            add_transaction(client, data);
          }
        } else if (fd.revents & (POLLERR | POLLNVAL | POLLHUP)) {
          terminate_connection(fd.fd, conf);
        }
      }
    }
  }

  close_server();
}
