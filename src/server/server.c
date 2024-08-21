#include "../../headers/telly.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

static uint32_t max_client_id_len;

static void epoll_ctl_add(int epfd, int fd, uint32_t events) {
  struct epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
    perror("epoll_ctl()");
    exit(EXIT_FAILURE);
  }
}

static int setnonblocking(int sockfd) {
  if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1) return -1;
  return 0;
}

void terminate_connection(struct epoll_event event, int epfd, struct Configuration conf) {
  struct Client *client = get_client(epfd);
  char message[41 + max_client_id_len];
  sprintf(message, "Client #%d is disconnected and terminated.", client->id);
  write_log(message, LOG_INFO, conf.allowed_log_levels);

  close(event.data.fd);
  remove_client(event.data.fd);
  epoll_ctl(epfd, EPOLL_CTL_DEL, event.data.fd, NULL);
}

void start_server(struct Configuration conf) {
  load_commands();
  pthread_t thread = create_transaction_thread(conf);

  int sockfd;
  struct sockaddr_in servaddr;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    write_log("cannot open socket", LOG_ERR, conf.allowed_log_levels);
    return;
  }

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(conf.port);

  if ((bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) != 0) { 
    write_log("cannot bind socket and address", LOG_ERR, conf.allowed_log_levels);
    return;
  }

  if (setnonblocking(sockfd) == -1) {
    write_log("cannot set non-blocking socket", LOG_ERR, conf.allowed_log_levels);
    return;
  }

  if (listen(sockfd, 10) != 0) { 
    write_log("cannot listen socket", LOG_ERR, conf.allowed_log_levels);
    return;
  }

  int epfd = epoll_create(1);
  epoll_ctl_add(epfd, sockfd, EPOLLIN | EPOLLOUT | EPOLLET);

  write_log("Server is ready for accepting connections...", LOG_INFO, conf.allowed_log_levels);

  struct epoll_event events[16];
  max_client_id_len = 1 + (uint32_t) log10(conf.max_clients);

  while (true) {
    int nfds = epoll_wait(epfd, events, 16, -1);

    for (int32_t i = 0; i < nfds; ++i) {
      struct epoll_event event = events[i];

      if (event.data.fd == sockfd) {
        if (conf.max_clients == get_client_count()) {
          char message[] = "A connection request is rejected, because connected client count to the server is maximum.";
          write_log(message, LOG_WARN, conf.allowed_log_levels);
        } else {
          struct sockaddr_in addr;
          socklen_t addr_len = sizeof(addr);

          const int connfd = accept(sockfd, (struct sockaddr *) &addr, &addr_len);
          struct Client *client = add_client(connfd, conf.max_clients);

          epoll_ctl_add(epfd, client->connfd, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP);

          char message[25 + max_client_id_len];
          sprintf(message, "A client #%d is connected.", client->id);
          write_log(message, LOG_INFO, conf.allowed_log_levels);
        }
      } else if (event.events & EPOLLIN) {
        struct Client *client = get_client(event.data.fd);
        const respdata_t data = get_resp_data(client->connfd);

        if (data.type == RDT_CLOSE) {
          terminate_connection(event, epfd, conf);
        } else {
          add_transaction(client, data);
        }
      } else if (event.events & (EPOLLRDHUP | EPOLLHUP)) {
        terminate_connection(event, epfd, conf);
      }
    }
  }

  pthread_cancel(thread);
  close(sockfd);
  close(epfd);
}
