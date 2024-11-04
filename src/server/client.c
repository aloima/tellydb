#include "../../headers/server.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

struct Client **clients;
uint32_t client_count = 0;
uint32_t last_connection_client_id = 0;

struct Password *default_password, *empty_password, *full_password;

void create_constant_passwords() {
  default_password = malloc(sizeof(struct Password));
  default_password->permissions = (P_READ | P_WRITE | P_CLIENT | P_CONFIG | P_AUTH | P_SERVER);

  empty_password = malloc(sizeof(struct Password));
  empty_password->permissions = 0;

  full_password = malloc(sizeof(struct Password));
  full_password->permissions = (P_READ | P_WRITE | P_CLIENT | P_CONFIG | P_AUTH | P_SERVER);
}

void free_constant_passwords() {
  free(default_password);
  free(empty_password);
  free(full_password);
}

struct Password *get_empty_password() {
  return empty_password;
}

struct Password *get_full_password() {
  return full_password;
}

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

struct Client *add_client(const int connfd) {
  client_count += 1;
  last_connection_client_id += 1;

  if (client_count == 1) {
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
  client->ssl = NULL;
  client->protover = RESP2;

  if (get_password_count() == 0) {
    client->password = default_password;
  } else {
    client->password = empty_password;
  }

  return client;
}

void remove_client(const int connfd) {
  for (uint32_t i = 0; i < client_count; ++i) {
    struct Client *client = clients[i];

    if (client->connfd == connfd) {
      client_count -= 1;

      if (client->ssl) SSL_free(client->ssl);
      if (client->lib_name) free(client->lib_name);
      if (client->lib_ver) free(client->lib_ver);
      free(client);

      if (client_count == 0) {
        free(clients);
      } else {
        memcpy(clients + i, clients + i + 1, (client_count - i) * sizeof(struct Client *));
        clients = realloc(clients, client_count * sizeof(struct Client));
      }

      break;
    }
  }
}
