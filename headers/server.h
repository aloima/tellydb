#include "config.h"
#include "resp.h"

#ifndef SERVER_H
  #define SERVER_H

  struct Client {
    int connfd;
    respdata_t *commands;
    uint32_t command_count;
  };

  void start_server(struct Configuration conf);

  struct Client *get_client(const int input);
  struct Client *add_client(const int connfd);
  void remove_client(const int connfd);

  void add_command_to_client(struct Client *client, respdata_t data);
#endif
