#include <telly.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <openssl/ssl.h>

struct LinkedListNode *head = NULL;
struct LinkedListNode *tail = NULL;

uint32_t client_count = 0;
uint32_t last_connection_client_id = 0;

struct Password *default_password = NULL, *empty_password = NULL, *full_password = NULL;

static bool create_constant_password(struct Password **password, uint64_t permissions) {
  if (posix_memalign((void **) password, 64, sizeof(struct Password)) != 0) {
    write_log(LOG_ERR, "Cannot create constant passwords, out of memory.");
    return false;
  }

  (*password)->permissions = permissions;
  return true;
}

bool create_constant_passwords() {
  const uint64_t permissions = (P_READ | P_WRITE | P_CLIENT | P_CONFIG | P_AUTH | P_SERVER);
  if (!create_constant_password(&default_password, permissions)) return false;
  if (!create_constant_password(&empty_password, 0)) return false;
  if (!create_constant_password(&full_password, permissions)) return false;

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

struct Client *get_client(const int input) {
  struct LinkedListNode *node = head;

  while (node) {
    struct Client *client = node->data;

    if (client->connfd == input) return client;
    else node = node->next;
  }

  return NULL;
}

struct Client *get_client_from_id(const uint32_t id) {
  struct LinkedListNode *node = head;

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
    head = malloc(sizeof(struct LinkedListNode));
    tail = head;
  } else {
    tail->next = malloc(sizeof(struct LinkedListNode));
    tail = tail->next;
  }

  tail->next = NULL;

  if (posix_memalign((void **) &tail->data, 64, sizeof(struct Client)) == 0) {
    struct Client *client = tail->data;
    client->id = last_connection_client_id;
    client->connfd = connfd;
    time(&client->connected_at);
    client->database = get_main_database();
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
  struct LinkedListNode *node = head;

  while (node) {
    struct Client *client = node->data;

    if (client->connfd == connfd) {
      client_count -= 1;

      if (client->ssl) SSL_free(client->ssl);
      if (client->lib_name) free(client->lib_name);
      if (client->lib_ver) free(client->lib_ver);
      free(client);

      if (client_count == 0) {
        free(head);
        head = NULL;
      } else {
        if (prev) prev->next = node->next;
        else head = node->next;

        free(node);
      }

      break;
    } else {
      prev = node;
      node = node->next;
    }
  }
}

struct LinkedListNode *get_head_client() {
  return head;
}

void remove_head_client() {
  struct Client *client = head->data;

  client_count -= 1;

  if (client->ssl) SSL_free(client->ssl);
  if (client->lib_name) free(client->lib_name);
  if (client->lib_ver) free(client->lib_ver);
  free(client);

  if (client_count == 0) {
    free(head);
    head = NULL;
  } else {
    struct LinkedListNode *prev = head;
    head = head->next;
    free(prev);
  }
}
