#include "../../headers/telly.h"

#include <stdint.h>
#include <stdlib.h>

struct Client **clients = NULL;
uint32_t client_count = 0;

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

struct Client *add_client(const int connfd, const uint32_t max_clients) {
  if (max_clients == client_count) {
    return NULL;
  }

  client_count += 1;

  if (clients == NULL) {
    clients = malloc(sizeof(struct Client *));
  } else {
    clients = realloc(clients, client_count * sizeof(struct Client));
  }

  const uint32_t li = client_count - 1;
  clients[li] = malloc(sizeof(struct Client));
  clients[li]->connfd = connfd;

  return clients[li];
}

void remove_client(const int connfd) {
  for (uint32_t i = 0; i < client_count; ++i) {
    struct Client *client = clients[i];

    if (client->connfd == connfd) {
      struct Client *last = clients[client_count - 1];
      memcpy(client, last, sizeof(struct Client));

      client_count -= 1;

      if (client_count == 0) {
        free(clients);
        clients = NULL;
      } else {
        clients = realloc(clients, client_count * sizeof(struct Client));
      }

      break;
    }
  }
}
