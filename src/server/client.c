#include <telly.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <time.h>

#include <openssl/ssl.h>

static _Atomic uint16_t client_count;
static _Atomic uint32_t last_connection_client_id;
static Arena *write_arena = NULL;

Client *get_client(const uint32_t id) {
  const Config *conf = server->conf;
  Client *clients = server->clients;

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

uint16_t get_client_count() {
  return atomic_load_explicit(&client_count, memory_order_relaxed);
}

uint32_t get_last_connection_client_id() {
  return atomic_load_explicit(&last_connection_client_id, memory_order_relaxed);
}

int initialize_clients() {
  const uint16_t max_clients = server->conf->max_clients;

  atomic_init(&client_count, 0);
  atomic_init(&last_connection_client_id, 1);

  write_arena = arena_create((max_clients / 4) * MAX_RESPONSE_SIZE * sizeof(char));
  if (write_arena == NULL) {
    write_log(LOG_ERR, "Cannot allocate writing buffer for clients, out of memory.");
    return -1;
  }

  if (amalloc(server->clients, Client, max_clients) != 0) {
    write_log(LOG_ERR, "Cannot create a map for storing clients, out of memory.");
    return -1;
  }

  Client *clients = server->clients;

  for (uint16_t i = 0; i < max_clients; ++i) {
    clients[i].id = -1;
    atomic_init(&clients[i].state, CLIENT_STATE_EMPTY);
  }

  return 0;
}

static inline uint16_t get_available_client_slot(const uint32_t id) {
  const uint16_t max_clients = server->conf->max_clients;
  const Client *clients = server->clients;

  uint16_t at = (id % max_clients);

  while (clients[at].id != -1) {
    at += 1;

    if (at == max_clients) {
      at = 0;
    }
  }

  return at;
}

Client *add_client(const int connfd) {
  const Config *conf = server->conf;
  Client *clients = server->clients;

  const uint32_t id = atomic_load_explicit(&last_connection_client_id, memory_order_relaxed);
  Client *client = &clients[get_available_client_slot(id)];

  client->id = id;
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

  client->write_buf = arena_alloc(write_arena, MAX_RESPONSE_SIZE * sizeof(char));
  if (client->write_buf == NULL) return NULL;

  if (get_password_count() == 0) {
    client->password = get_default_password();
  } else {
    client->password = get_empty_password();
  }

  atomic_init(&client.state, CLIENT_STATE_ACTIVE);
  atomic_fetch_add_explicit(&client_count, 1, memory_order_relaxed);
  atomic_fetch_add_explicit(&last_connection_client_id, 1, memory_order_relaxed);

  return client;
}

static inline void free_client(Client *client) {
  if (client->ssl) {
    SSL_shutdown(client->ssl);
    SSL_free(client->ssl);
  }

  if (client->lib_name) free(client->lib_name);
  if (client->lib_ver) free(client->lib_ver);
  if (client->waiting_block) remove_transaction_block(client->waiting_block);
}

bool remove_client(const int id) {
  const Config *conf = server->conf;
  Client *clients = server->clients;

  uint16_t at = (id % conf->max_clients);
  Client *client;

  while ((client = &clients[at])->id != id) {
    at += 1;

    if (at == conf->max_clients) {
      at = 0;
    }
  }

  if (atomic_load_explicit(&client->state, memory_order_acquire) == CLIENT_STATE_EMPTY) return false;

  client->id = -1;
  free_client(client);
  atomic_fetch_sub_explicit(&client_count, 1, memory_order_relaxed);

  atomic_store_explicit(&client->state, CLIENT_STATE_EMPTY, memory_order_release);
  return true;
}

void free_clients() {
  const uint16_t max_clients = server->conf->max_clients;
  Client *clients = server->clients;

  for (uint16_t i = 0; i < max_clients; ++i) {
    Client *client = &clients[i];
    if (atomic_load_explicit(&client->state, memory_order_acquire) == CLIENT_STATE_EMPTY) continue;
    free_client(client);
  }

  arena_destroy(write_arena);
  free(clients);
}
