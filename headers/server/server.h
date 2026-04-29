#pragma once

#include "client.h"
#include "macros.h" // IWYU pragma: export

#include "../config.h"
#include "../utils/utils.h"

#include <signal.h>
#include <stdint.h>
#include <time.h>

#include <sys/types.h>

#include <openssl/crypto.h>

#include "io.h"     // IWYU pragma: export
#include "macros.h" // IWYU pragma: export
#include "client.h" // IWYU pragma: export

#define INITIAL_UNKNOWN_COMMAND_ARENA_SIZE 8192

typedef enum {
  SERVER_STATUS_NONE = 0,
  SERVER_STATUS_STARTING,
  SERVER_STATUS_ONLINE,
  SERVER_STATUS_ERROR,
  SERVER_STATUS_CLOSED
} ServerStatus;

typedef struct {
  int eventfd;
  int sockfd;

  SSL_CTX *ctx;
  Config *conf;
  time_t start_at;
  uint32_t age;
  time_t last_error_at;
  ServerStatus status;
  sig_atomic_t closed;
  struct Command *commands;
  Client *clients;

  // Given keys in the command, one keyspace is enough because of that transactions is ran individually.
  Vector *keyspace;
} Server;

extern Server *server;

void terminate_connection(Client *client);
off_t *get_authorization_end_at();
void get_server_time(time_t *server_start_at, uint32_t *server_age);
void handle_events();
void start_server(Config *config);

int read_from_socket(Client *client, char *buf, const size_t nbytes);
int write_to_socket(Client *client, char *buf, const size_t nbytes);
string_t write_value(void *value, const enum TellyTypes type, const enum ProtocolVersion protover, char *buffer);

void read_command(IOThread *thread, Client *client);
