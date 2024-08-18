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
    clients = malloc(sizeof(struct Client));
  } else {
    clients = realloc(clients, client_count * sizeof(struct Client));
  }

  const uint32_t li = client_count - 1;
  clients[li] = malloc(sizeof(struct Client));
  clients[li]->connfd = connfd;
  clients[li]->commands = NULL;
  clients[li]->command_count = 0;

  return clients[li];
}

void remove_client(const int connfd) {
  for (uint32_t i = 0; i < client_count; ++i) {
    struct Client *client = clients[i];

    if (client->connfd == connfd) {
      struct Client *last = clients[client_count - 1];

      if (client->commands == NULL) {
        client->commands = malloc(last->command_count * sizeof(respdata_t));
      } else {
        for (uint32_t j = 0; j < client->command_count; ++j) {
          free(client->commands[j]);
        }

        client->commands = realloc(client->commands, last->command_count * sizeof(respdata_t));
      }

      client->connfd = last->connfd;
      client->command_count = last->command_count;
      memcpy(client->commands, last->commands, last->command_count);

      free(last->commands);
      free(last);

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

void add_command_to_client(struct Client *client, respdata_t data) {
  client->command_count += 1;

  if (client->commands == NULL) {
    client->commands = malloc(sizeof(respdata_t *));
  } else {
    client->commands = realloc(client->commands, client->command_count * sizeof(respdata_t *));
  }

  const uint32_t last = client->command_count - 1;
  client->commands[last] = malloc(sizeof(respdata_t));
  memcpy(client->commands[last], &data, sizeof(respdata_t));
}

void remove_command_from_client(struct Client *client, respdata_t *data) {
  for (uint32_t i = 0; i < client->command_count; ++i) {
    if (data == client->commands[i]) {
      client->command_count -= 1;
      memcpy(client->commands + i, client->commands + i + 1, sizeof(respdata_t) * client->command_count);
      free(client->commands[client->command_count]);

      if (client->command_count == 0) {
        free(client->commands);
        client->commands = NULL;
      } else {
        client->commands = realloc(client->commands, client->command_count * sizeof(respdata_t *));
      }

      break;
    }
  }
}
