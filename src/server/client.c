#include <telly.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <openssl/ssl.h>

static struct Client *clients = NULL;
static int64_t *connfd_client_pos = NULL; // connfd => client index on `clients`
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
  const uint32_t start = (input % client_capacity);
  uint32_t at = start;

  while (connfd_client_pos[at] != -1) {
    if (clients[connfd_client_pos[at]].connfd == input) {
      return &clients[connfd_client_pos[at]];
    }

    at += 1;

    if (at == client_capacity) {
      at = 0;
    }

    if (at == start) {
      return NULL;
    }
  }

  return NULL;
}

struct Client *get_client_from_id(const uint32_t id) {
  const uint32_t start = (id % client_capacity);
  uint32_t at = at;

  while (clients[at].id != -1) {
    if (clients[at].id == id) {
      return &clients[at];
    }

    at += 1;

    if (at == client_capacity) {
      at = 0;
    }

    if (at == start) {
      return NULL;
    }
  }

  return NULL;
}

struct Client *get_clients() {
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
  const size_t size = (sizeof(struct Client) * client_capacity);

  if (posix_memalign((void **) &clients, 16, size) != 0) {
    write_log(LOG_ERR, "Cannot create a map for storing clients, out of memory.");
    return false;
  }

  if (posix_memalign((void **) &connfd_client_pos, 16, size) != 0) {
    write_log(LOG_ERR, "Cannot create a map for storing clients, out of memory.");
    return false;
  }

  const int64_t val = -1;

  for (uint32_t i = 0; i < client_capacity; ++i) {
    clients[i].id = -1;
    connfd_client_pos[i] = -1;
  }

  return true;
}

static inline struct Client *insert_client(struct Client client) {
  uint32_t at = (client.id % client_capacity);

  while (clients[at].id != -1) {
    at += 1;

    if (at == client_capacity) {
      at = 0;
    }
  }

  clients[at] = client;

  const uint32_t idx_of_client = at;
  at = (client.connfd % client_capacity);

  while (connfd_client_pos[at] != -1) {
    at += 1;

    if (at == client_capacity) {
      at = 0;
    }
  }

  connfd_client_pos[at] = idx_of_client;
  return &clients[idx_of_client];
}

static inline bool resize_client_maps() {
  client_capacity += client_capacity;

  const uint32_t size = (sizeof(struct Client) * client_capacity);
  struct Client *old_clients = clients;
  free(connfd_client_pos);

  if (posix_memalign((void **) &clients, 16, size) != 0) {
    write_log(LOG_ERR, "Cannot resize a map for storing clients, out of memory.");
    return false;
  }

  if (posix_memalign((void **) &connfd_client_pos, 16, size) != 0) {
    write_log(LOG_ERR, "Cannot resize a map for storing clients, out of memory.");
    return false;
  }

  const int64_t val = -1;

  for (uint32_t i = 0; i < client_capacity; ++i) {
    clients[i].id = -1;
    connfd_client_pos[i] = -1;
  }

  for (uint32_t i = 0; i < client_count; ++i) {
    insert_client(old_clients[i]);
  }

  return true;
}

struct Client *add_client(const int connfd) {
  struct Client client;

  if (client_count == client_capacity) {
    if (!resize_client_maps()) {
      return NULL;
    }
  }

  client_count += 1;
  last_connection_client_id += 1;

  client.id = last_connection_client_id;
  client.connfd = connfd;
  time(&client.connected_at);
  client.database = get_main_database();
  client.command = NULL;
  client.lib_name = NULL;
  client.lib_ver = NULL;
  client.ssl = NULL;
  client.protover = RESP2;
  client.locked = false;
  client.waiting_block = NULL;

  if (get_password_count() == 0) {
    client.password = default_password;
  } else {
    client.password = empty_password;
  }

  return insert_client(client);
}

bool remove_client(const int connfd) {
  int64_t at = (connfd % client_capacity);

  if (connfd_client_pos[at] == -1) {
    return false;
  }

  struct Client client = clients[connfd_client_pos[at]];

  while (client.connfd != connfd) {
    at += 1;

    if (at == client_capacity) {
      at = 0;
    }

    client = clients[connfd_client_pos[at]];

    if (connfd_client_pos[at] == -1) {
      return false;
    }
  }

  connfd_client_pos[at] = -1;

  const uint32_t id = client.id;
  at = (client.id % client_capacity);
  client = clients[at];

  while (clients[at].id != id) {
    at += 1;

    if (at == client_capacity) {
      at = 0;
    }
  }

  clients[at].id = -1;
  client_count -= 1;

  if (client.ssl) {
    SSL_shutdown(client.ssl);
    SSL_free(client.ssl);
  }

  if (client.lib_name) {
    free(client.lib_name);
  }

  if (client.lib_ver) {
    free(client.lib_ver);
  }

  if (client.waiting_block) {
    remove_transaction_block(client.waiting_block, false);
  }

  return true;
}

void free_client_maps() {
  for (uint32_t i = 0; i < client_capacity; ++i) {
    if (clients[i].id != -1) {
      remove_client(clients[i].connfd);
    }
  }

  free(connfd_client_pos);
  free(clients);
}
