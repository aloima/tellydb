#include "config.h"
#include "resp.h"

#include <stdint.h>

#ifndef SERVER_H
  #define SERVER_H

  struct Client {
    int connfd;
    respdata_t **commands;
    uint32_t command_count;
  };

  void start_server(struct Configuration conf);

  struct Client **get_clients();
  struct Client *get_client(const int input);
  uint32_t get_client_count();
  struct Client *add_client(const int connfd, const uint32_t max_clients);
  void remove_client(const int connfd);

  void add_command_to_client(struct Client *client, respdata_t data);
  void remove_command_from_client(struct Client *client, respdata_t *data);
#endif
