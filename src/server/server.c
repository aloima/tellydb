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

void terminate_connection(const int connfd) {
  struct Client *client = get_client(connfd);
  write_log(LOG_INFO, "Client #%d is disconnected.", client->id);

  for (uint32_t i = 1; i < nfds; ++i) {
    if (fds[i].fd == connfd) {
      memcpy(fds + i, fds + i + 1, (nfds - i - 1) * sizeof(struct pollfd));
      break;
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

  for (uint32_t i = 0; i < client_count; ++i) {
    struct Client *client = clients[0];
    write_log(LOG_INFO, "Client #%d is terminated.", client->id);

    close(client->connfd);
    remove_client(client->connfd);
  }

  write_log(LOG_WARN, "Saving data...");
  save_data();
  close_database_file();
  write_log(LOG_INFO, "Saved data and closed database file.");

  deactive_transaction_thread();
  usleep(15);
  write_log(LOG_INFO, "Exited transaction thread.");

  free_transactions();
  free_commands();
  free_cache();
  close(sockfd);
  free(fds);
  write_log(LOG_INFO, "Free'd all memory blocks and closed the server.");

  free(conf);
  exit(EXIT_SUCCESS);
}

static void sigint_signal(__attribute__((unused)) int arg) {
  write_log(LOG_WARN, "Received SIGINT signal, closing the server...", LOG_WARN);
  close_server();
}

void start_server(struct Configuration *config) {
  conf = config;
  initialize_logs(conf);
  write_log(LOG_INFO, "Initialized logs and configuration.");

  load_commands();
  write_log(LOG_INFO, "Initialized commands.");

  thread = create_transaction_thread(config);
  write_log(LOG_INFO, "Created transaction thread.");

  signal(SIGINT, sigint_signal);

  struct sockaddr_in servaddr;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    deactive_transaction_thread();
    usleep(15);
    free_commands();
    write_log(LOG_ERR, "Cannot open socket, safely exiting...");
    free(conf);
    return;
  }

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(conf->port);

  if ((bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) != 0) { 
    deactive_transaction_thread();
    usleep(15);
    free_commands();
    close(sockfd);
    write_log(LOG_ERR, "Cannot bind socket and address, safely exiting...");
    free(conf);
    return;
  }

  if (setnonblocking(sockfd) == -1) {
    deactive_transaction_thread();
    usleep(15);
    free_commands();
    close(sockfd);
    write_log(LOG_ERR, "Cannot set non-blocking socket, safely exiting...");
    free(conf);
    return;
  }

  if (listen(sockfd, 10) != 0) { 
    deactive_transaction_thread();
    usleep(15);
    free_commands();
    close(sockfd);
    write_log(LOG_ERR, "Cannot listen socket, safely exiting...");
    free(conf);
    return;
  }

  create_cache();
  open_database_file(conf->data_file);
  write_log(LOG_INFO, "Created cache and opened database file.");

  nfds = 1;
  fds = malloc(sizeof(struct pollfd));
  fds[0].fd = sockfd;
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  write_log(LOG_INFO, "Server is listening on %d port for accepting connections...", conf->port);

  max_client_id_len = get_digit_count(conf->max_clients);

  while (true) {
    int ret = poll(fds, nfds, -1);

    if (ret != -1) {
      for (uint32_t i = 0; i < nfds; ++i) {
        struct pollfd fd = fds[i];

        if (fd.fd == sockfd && fd.revents & POLLIN) {
          if (conf->max_clients == get_client_count()) {
            write_log(LOG_WARN, "A connection request is rejected, because connected client count to the server is maximum.");
          } else {
            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);

            const int connfd = accept(sockfd, (struct sockaddr *) &addr, &addr_len);
            struct Client *client = add_client(connfd, conf->max_clients);

            nfds += 1;
            fds = realloc(fds, nfds * sizeof(struct pollfd));

            const uint32_t at = nfds - 1;
            fds[at].fd = connfd;
            fds[at].events = POLLIN;
            fds[at].revents = 0;

            write_log(LOG_INFO, "Client #%d is connected.", client->id);
          }
        } else if (fd.revents & POLLIN) {
          respdata_t *data = get_resp_data(fd.fd);

          if (data->type == RDT_CLOSE) {
            terminate_connection(fd.fd);
            free_resp_data(data);
          } else {
            struct Client *client = get_client(fd.fd);
            struct Command *commands = get_commands();
            const uint32_t command_count = get_command_count();

            string_t name = data->value.array[0]->value.string;

            char used[name.len + 1];
            to_uppercase(name.value, used);

            for (uint32_t i = 0; i < command_count; ++i) {
              if (streq(commands[i].name, used)) {
                client->command = &commands[i];
                break;
              }
            }

            add_transaction(client, data);
          }
        } else if (fd.revents & (POLLERR | POLLNVAL | POLLHUP)) {
          terminate_connection(fd.fd);
        }
      }
    }
  }

  close_server();
}
