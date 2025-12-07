#include <telly.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <time.h>

#include <openssl/ssl.h>

static struct Configuration *conf;

static Client *clients = NULL;
static _Atomic uint16_t client_count;
static _Atomic uint32_t last_connection_client_id;

struct Password *default_password = NULL,
                *empty_password = NULL,
                *full_password = NULL;

static bool create_constant_password(struct Password **password, uint64_t permissions) {
  if (posix_memalign((void **) password, 64, sizeof(struct Password)) != 0) {
    write_log(LOG_ERR, "Cannot create constant passwords, out of memory.");
    return false;
  }

  (*password)->permissions = permissions;
  return true;
}

bool create_constant_passwords() {
  constexpr uint64_t permissions = (P_READ | P_WRITE | P_CLIENT | P_CONFIG | P_AUTH | P_SERVER);
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

Client *get_client(const uint32_t id) {
  const uint32_t start = (id % conf->max_clients);
  uint32_t at = start;

  while (clients[at].id != -1) {
    if (clients[at].id == id) return &clients[at];

    at += 1;
    if (at == conf->max_clients) at = 0;

    if (at == start) {
      return NULL;
    }
  }

  return NULL;
}

Client *get_clients() {
  return clients;
}

uint16_t get_client_count() {
  return atomic_load_explicit(&client_count, memory_order_relaxed);
}

uint32_t get_last_connection_client_id() {
  return atomic_load_explicit(&last_connection_client_id, memory_order_relaxed);
}

int initialize_clients() {
  conf = get_server_configuration();
  atomic_init(&client_count, 0);
  atomic_init(&last_connection_client_id, 1);

  const size_t size = (sizeof(Client) * conf->max_clients);

  if (posix_memalign((void **) &clients, 16, size) != 0) {
    write_log(LOG_ERR, "Cannot create a map for storing clients, out of memory.");
    return -1;
  }

  for (uint32_t i = 0; i < conf->max_clients; ++i) {
    clients[i].id = -1;
  }

  return 0;
}

static inline Client *insert_client(Client *client) {
  uint16_t at = (client->id % conf->max_clients);

  while (clients[at].id != -1) {
    at += 1;

    if (at == conf->max_clients) {
      at = 0;
    }
  }

  clients[at] = *client;
  return &clients[at];
}

Client *add_client(const int connfd) {
  atomic_fetch_add_explicit(&client_count, 1, memory_order_relaxed);
  
  Client client;
  client.id = atomic_fetch_add_explicit(&last_connection_client_id, 1, memory_order_relaxed);
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
  client.write_buf = malloc(MAX_RESPONSE_SIZE * sizeof(char));
  atomic_init(&client.state, CLIENT_STATE_ACTIVE);

  if (get_password_count() == 0) {
    client.password = default_password;
  } else {
    client.password = empty_password;
  }

  return insert_client(&client);
}

bool remove_client(const int id) {
  uint16_t at = (id % conf->max_clients);
  Client *client;

  while ((client = &clients[at])->id != id) {
    at += 1;

    if (at == conf->max_clients) {
      at = 0;
    }
  }

  client->id = -1;
  atomic_fetch_sub_explicit(&client_count, 1, memory_order_relaxed);

  if (client->ssl) {
    SSL_shutdown(client->ssl);
    SSL_free(client->ssl);
  }

  free(client->write_buf);
  if (client->lib_name) free(client->lib_name);
  if (client->lib_ver) free(client->lib_ver);
  if (client->waiting_block) remove_transaction_block(client->waiting_block);
  atomic_store_explicit(&client->state, CLIENT_STATE_EMPTY, memory_order_release);

  return true;
}

void free_clients() {
  for (uint16_t i = 0; i < conf->max_clients; ++i) {
    if (clients[i].id != -1) {
      remove_client(clients[i].connfd);
    }
  }

  free(clients);
}
