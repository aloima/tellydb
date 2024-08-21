#include "config.h"
#include "resp.h"

#include <stdint.h>

#include <sys/epoll.h>

#ifndef SERVER_H
  #define SERVER_H

  struct Client {
    int connfd;
    uint32_t id;
    time_t connected_at;
    struct Command *command;
  };

  void start_server(struct Configuration conf);
  void terminate_connection(struct epoll_event event, int epfd, struct Configuration conf);

  struct Client **get_clients();
  struct Client *get_client(const int input);

  uint32_t get_last_connection_client_id();
  uint32_t get_client_count();

  struct Client *add_client(const int connfd, const uint32_t max_clients);
  void remove_client(const int connfd);
#endif
