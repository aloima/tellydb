#include <telly.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <openssl/ssl.h>

static struct Client **clients = NULL;
static struct Client **connfd_clients = NULL;
static uint32_t client_capacity = 16;
static uint32_t client_count = 0;

static uint32_t last_connection_client_id = 0;

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

  if (!create_constant_password(&default_password, permissions)) {
    return false;
  }

  if (!create_constant_password(&empty_password, 0)) {
    return false;
  }

  if (!create_constant_password(&full_password, permissions)) {
    return false;
  }

  return true;
}

void free_constant_passwords() {
  if (default_password) {
    free(default_password);
  }

  if (empty_password) {
    free(empty_password);
  }

  if (full_password) {
    free(full_password);
  }
}

struct Password *get_empty_password() {
  return empty_password;
}

struct Password *get_full_password() {
  return full_password;
}

struct Client *get_client(const int input) {
  struct Client *client;
  uint32_t at = (input % client_capacity);

  while ((client = connfd_clients[at])) {
    if (client->connfd == input) {
      return client;
    }

    at += 1;

    if (at == client_capacity) {
      return NULL;
    }
  }

  return clients[at];
}

struct Client *get_client_from_id(const uint32_t id) {
  struct Client *client;
  uint32_t at = (id % client_capacity);

  while ((client = clients[at])) {
    if (client->id == id) {
      return client;
    }

    at += 1;

    if (at == client_capacity) {
      return NULL;
    }
  }

  return client;
}

struct Client **get_clients() {
  return clients;
}

uint32_t get_client_count() {
  return client_count;
}

uint32_t get_client_capacity() {
  return client_capacity;
}

uint32_t get_last_connection_client_id() {
  return last_connection_client_id;
}

bool initialize_client_maps() {
  const size_t size = (sizeof(struct Client *) * client_capacity);

  if (posix_memalign((void **) &clients, 16, size) != 0) {
    write_log(LOG_ERR, "Cannot create a map for storing clients, out of memory.");
    return false;
  }

  if (posix_memalign((void **) &connfd_clients, 16, size) != 0) {
    write_log(LOG_ERR, "Cannot create a map for storing clients, out of memory.");
    return false;
  }

  memset(clients, 0, size);
  memset(connfd_clients, 0, size);
  return true;
}

static inline void insert_client(struct Client *client) {
  uint32_t at = (client->id % client_capacity);

  while (clients[at]) {
    at = ((at + 1) % client_capacity);
  }

  clients[at] = client;

  at = (client->connfd % client_capacity);

  while (connfd_clients[at]) {
    at = ((at + 1) % client_capacity);
  }

  connfd_clients[at] = client;
}

static inline bool resize_client_maps() {
  client_capacity += client_capacity;

  const uint32_t size = (sizeof(struct Client *) * client_capacity);
  struct Client **old_clients = clients;
  free(connfd_clients);

  if (posix_memalign((void **) &clients, 16, size) != 0) {
    write_log(LOG_ERR, "Cannot resize a map for storing clients, out of memory.");
    return false;
  }

  if (posix_memalign((void **) &connfd_clients, 16, size) != 0) {
    write_log(LOG_ERR, "Cannot resize a map for storing clients, out of memory.");
    return false;
  }

  memset(clients, 0, size);
  memset(connfd_clients, 0, size);

  for (uint32_t i = 0; i < client_count; ++i) {
    insert_client(old_clients[i]);
  }

  free(old_clients);
  return true;
}

struct Client *add_client(const int connfd) {
  struct Client *client;

  if (posix_memalign((void **) &client, 16, sizeof(struct Client)) != 0) {
    write_log(LOG_ERR, "Cannot create a new client, out of memory.");
    return NULL;
  }

  if (client_count == client_capacity) {
    if (!resize_client_maps()) {
      return NULL;
    }
  }

  client_count += 1;
  last_connection_client_id += 1;

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
  client->waiting_block = NULL;

  if (get_password_count() == 0) {
    client->password = default_password;
  } else {
    client->password = empty_password;
  }

  insert_client(client);
  return client;
}

bool remove_client(const int connfd) {
  uint32_t at = (connfd % client_capacity);
  struct Client *client = connfd_clients[at];

  if (!client) {
    return false;
  }

  while (client->connfd != connfd) {
    at = ((at + 1) % client_capacity);
    client = connfd_clients[at];

    if (!client) {
      return false;
    }
  }

  const uint32_t id = client->id;
  connfd_clients[at] = NULL;
  at = (client->id % client_capacity);
  client = clients[at];

  while (client->id != id) {
    at = ((at + 1) % client_capacity);
    client = clients[at];
  }

  clients[at] = NULL;
  client_count -= 1;

  if (client->ssl) {
    SSL_shutdown(client->ssl);
    SSL_free(client->ssl);
  }

  if (client->lib_name) {
    free(client->lib_name);
  }

  if (client->lib_ver) {
    free(client->lib_ver);
  }

  if (client->waiting_block) {
    remove_transaction_block(client->waiting_block, false);
  }

  free(client);
  return true;
}

void free_client_maps() {
  for (uint32_t i = 0; i < client_capacity; ++i) {
    if (clients[i]) {
      remove_client(clients[i]->connfd);
    }
  }

  free(connfd_clients);
  free(clients);
}
