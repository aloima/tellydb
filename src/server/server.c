#include "../../headers/telly.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

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

void terminate_connection(struct epoll_event event, int epfd) {
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
    fputs("cannot open socket\n", stderr);
    return;
  }

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(conf.port);

  if ((bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) != 0) { 
    fputs("cannot bind socket\n", stderr);
    return;
  }

  if (setnonblocking(sockfd) == -1) {
    fputs("cannot set non blocking socket\n", stderr);
    return;
  }

  if (listen(sockfd, 10) != 0) { 
    fputs("cannot listen socket\n", stderr);
    return;
  }

  int epfd = epoll_create(1);
  epoll_ctl_add(epfd, sockfd, EPOLLIN | EPOLLOUT | EPOLLET);

  struct epoll_event events[16];

  while (true) {
    int nfds = epoll_wait(epfd, events, 16, -1);

    for (int32_t i = 0; i < nfds; ++i) {
      struct epoll_event event = events[i];

      if (event.data.fd == sockfd) {
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);

        const int connfd = accept(sockfd, (struct sockaddr *) &addr, &addr_len);
        struct Client *client = add_client(connfd, conf.max_clients);

        epoll_ctl_add(epfd, client->connfd, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP);
      } else if (event.events & EPOLLIN) {
        struct Client *client = get_client(event.data.fd);
        const respdata_t data = get_resp_data(client->connfd);

        if (data.type == RDT_CLOSE) {
          terminate_connection(event, epfd);
        } else {
          add_transaction(client, data);
        }
      } else if (event.events & (EPOLLRDHUP | EPOLLHUP)) {
        terminate_connection(event, epfd);
      }
    }
  }

  pthread_cancel(thread);
  close(sockfd);
  close(epfd);
}
