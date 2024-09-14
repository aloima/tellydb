#include "../../headers/telly.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

struct Client **clients = NULL;
uint32_t client_count = 0;
uint32_t last_connection_client_id = 0;

struct Client **get_clients() {
  return clients;
}

struct Client *get_client(const int input) {
  for (uint32_t i = 0; i < client_count; ++i) {
    struct Client *client = clients[i];

    if (client->connfd == input) {
      return client;
    }
  }

  return NULL;
}

uint32_t get_client_count() {
  return client_count;
}

uint32_t get_last_connection_client_id() {
  return last_connection_client_id;
}

struct Client *add_client(const int connfd, const uint32_t max_clients) {
  if (max_clients == client_count) {
    return NULL;
  }

  client_count += 1;
  last_connection_client_id += 1;

  if (clients == NULL) {
    clients = malloc(sizeof(struct Client *));
  } else {
    clients = realloc(clients, client_count * sizeof(struct Client *));
  }

  clients[client_count - 1] = malloc(sizeof(struct Client));

  struct Client *client = clients[client_count - 1];
  client->id = last_connection_client_id;
  client->connfd = connfd;
  time(&client->connected_at);
  client->command = NULL;
  client->lib_name = NULL;
  client->lib_ver = NULL;

  return client;
}

void remove_client(const int connfd) {
  for (uint32_t i = 0; i < client_count; ++i) {
    struct Client *client = clients[i];

    if (client->connfd == connfd) {
      client_count -= 1;

      if (client->lib_name) free(client->lib_name);
      if (client->lib_ver) free(client->lib_ver);
      free(client);

      if (client_count == 0) {
        free(clients);
        clients = NULL;
      } else {
        memcpy(clients + i, clients + i + 1, (client_count - i) * sizeof(struct Client *));
        clients = realloc(clients, client_count * sizeof(struct Client));
      }

      break;
    }
  }
}
