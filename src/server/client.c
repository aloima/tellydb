#include "../../headers/server.h"
#include "../../headers/utils.h"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

struct LinkedListNode *nodes = NULL;
struct LinkedListNode *last_node = NULL;

uint32_t client_count = 0;
uint32_t last_connection_client_id = 0;

struct Password *default_password = NULL, *empty_password = NULL, *full_password = NULL;

#define alloc_constant_password(password) if (posix_memalign(password, 64, sizeof(struct Password)) != 0) {\
  write_log(LOG_ERR, "Cannot create constant passwords, out of memory.");\
  return false;\
}

bool create_constant_passwords() {
  alloc_constant_password((void **) &default_password);
  default_password->permissions = (P_READ | P_WRITE | P_CLIENT | P_CONFIG | P_AUTH | P_SERVER);

  alloc_constant_password((void **) &empty_password);
  empty_password->permissions = 0;

  alloc_constant_password((void **) &full_password);
  full_password->permissions = (P_READ | P_WRITE | P_CLIENT | P_CONFIG | P_AUTH | P_SERVER);

  return true;
}

void free_constant_passwords() {
  if (default_password) free(default_password);
  if (empty_password) free(empty_password);
  if (full_password) free(full_password);
}

struct Password *get_empty_password() {
  return empty_password;
}

struct Password *get_full_password() {
  return full_password;
}

struct LinkedListNode *get_client_nodes() {
  return nodes;
}

struct Client *get_client(const int input) {
  struct LinkedListNode *node = nodes;

  while (node) {
    struct Client *client = node->data;

    if (client->connfd == input) return client;
    else node = node->next;
  }

  return NULL;
}

struct Client *get_client_from_id(const uint32_t id) {
  struct LinkedListNode *node = nodes;

  while (node) {
    struct Client *client = node->data;

    if (client->id == id) return client;
    else node = node->next;
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
    nodes = malloc(sizeof(struct LinkedListNode));
    last_node = nodes;
  } else {
    last_node->next = malloc(sizeof(struct LinkedListNode));
    last_node = last_node->next;
  }

  last_node->next = NULL;

  if (posix_memalign((void **) &last_node->data, 64, sizeof(struct Client)) == 0) {
    struct Client *client = last_node->data;
    client->id = last_connection_client_id;
    client->connfd = connfd;
    time(&client->connected_at);
    client->command = NULL;
    client->lib_name = NULL;
    client->lib_ver = NULL;
    client->ssl = NULL;
    client->protover = RESP2;
    client->locked = false;

    if (get_password_count() == 0) {
      client->password = default_password;
    } else {
      client->password = empty_password;
    }

    return client;
  } else {
    write_log(LOG_ERR, "Cannot create a new client, out of memory.");
    return NULL;
  }
}

void remove_client(const int connfd) {
  struct LinkedListNode *prev = NULL;
  struct LinkedListNode *node = nodes;

  while (node) {
    struct Client *client = node->data;

    if (client->connfd == connfd) {
      client_count -= 1;

      if (client->ssl) SSL_free(client->ssl);
      if (client->lib_name) free(client->lib_name);
      if (client->lib_ver) free(client->lib_ver);
      free(client);

      if (client_count == 0) {
        free(nodes);
        nodes = NULL;
      } else {
        if (prev) prev->next = node->next;
        else nodes = node->next;

        free(node);
      }

      break;
    } else {
      prev = node;
      node = node->next;
    }
  }
}

struct Client *get_first_client() {
  return (nodes ? nodes->data : NULL);
}

void remove_first_client() {
  struct Client *client = nodes->data;

  client_count -= 1;

  if (client->ssl) SSL_free(client->ssl);
  if (client->lib_name) free(client->lib_name);
  if (client->lib_ver) free(client->lib_ver);
  free(client);

  if (client_count == 0) {
    free(nodes);
    nodes = NULL;
  } else {
    struct LinkedListNode *prev = nodes;
    nodes = nodes->next;
    free(prev);
  }
}
