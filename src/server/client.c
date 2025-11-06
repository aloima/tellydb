#include <telly.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <openssl/ssl.h>

// TODO: make thread-safe
static struct Configuration *conf = NULL;
static struct Client *clients = NULL;
static int32_t *connfd_client_pos = NULL; // connfd => client index on `clients`
static uint16_t client_count = 0;

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
  const uint32_t start = (input % conf->max_clients);
  uint32_t at = start;

  while (connfd_client_pos[at] != -1) {
    if (clients[connfd_client_pos[at]].connfd == input) {
      return &clients[connfd_client_pos[at]];
    }

    at += 1;

    if (at == conf->max_clients) {
      at = 0;
    }

    if (at == start) {
      return NULL;
    }
  }

  return NULL;
}

struct Client *get_client_from_id(const uint32_t id) {
  const uint32_t start = (id % conf->max_clients);
  uint32_t at = start;

  while (clients[at].id != -1) {
    if (clients[at].id == id) {
      return &clients[at];
    }

    at += 1;

    if (at == conf->max_clients) {
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

uint16_t get_client_count() {
  return client_count;
}

uint32_t get_last_connection_client_id() {
  return last_connection_client_id;
}

bool initialize_client_maps() {
  conf = get_server_configuration();
  const size_t size = (sizeof(struct Client) * conf->max_clients);

  if (posix_memalign((void **) &clients, 16, size) != 0) {
    write_log(LOG_ERR, "Cannot create a map for storing clients, out of memory.");
    return false;
  }

  if ((connfd_client_pos = malloc(sizeof(int32_t) * conf->max_clients)) == NULL) {
    write_log(LOG_ERR, "Cannot create a map for storing clients, out of memory.");
    return false;
  }

  for (uint32_t i = 0; i < conf->max_clients; ++i) {
    clients[i].id = -1;
    connfd_client_pos[i] = -1;
  }

  return true;
}

static inline struct Client *insert_client(struct Client client) {
  uint16_t at = (client.id % conf->max_clients);

  while (clients[at].id != -1) {
    at += 1;

    if (at == conf->max_clients) {
      at = 0;
    }
  }

  clients[at] = client;

  const uint16_t idx_of_client = at;
  at = (client.connfd % conf->max_clients);

  while (connfd_client_pos[at] != -1) {
    at += 1;

    if (at == conf->max_clients) {
      at = 0;
    }
  }

  connfd_client_pos[at] = idx_of_client;
  return &clients[idx_of_client];
}

struct Client *add_client(const int connfd) {
  struct Client client;

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
  uint16_t at = (connfd % conf->max_clients);

  if (connfd_client_pos[at] == -1) {
    return false;
  }

  struct Client client = clients[connfd_client_pos[at]];

  while (client.connfd != connfd) {
    at += 1;

    if (at == conf->max_clients) {
      at = 0;
    }

    client = clients[connfd_client_pos[at]];

    if (connfd_client_pos[at] == -1) {
      return false;
    }
  }

  connfd_client_pos[at] = -1;

  const uint16_t id = client.id;
  at = (client.id % conf->max_clients);
  client = clients[at];

  while (clients[at].id != id) {
    at += 1;

    if (at == conf->max_clients) {
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
  for (uint16_t i = 0; i < conf->max_clients; ++i) {
    if (clients[i].id != -1) {
      remove_client(clients[i].connfd);
    }
  }

  free(connfd_client_pos);
  free(clients);
}
