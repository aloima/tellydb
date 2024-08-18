#include "config.h"
#include "resp.h"

#include <stdint.h>

#include <sys/epoll.h>

#ifndef SERVER_H
  #define SERVER_H

  struct Client {
    int connfd;
  };

  void start_server(struct Configuration conf);
  void terminate_connection(struct epoll_event event, int epfd);

  struct Client **get_clients();
  struct Client *get_client(const int input);
  uint32_t get_client_count();
  struct Client *add_client(const int connfd, const uint32_t max_clients);
  void remove_client(const int connfd);
#endif
