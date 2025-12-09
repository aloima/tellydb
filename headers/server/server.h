#pragma once

#include "client.h" // IWYU pragma: export
#include "macros.h" // IWYU pragma: export

#include "../config.h"
#include "../utils/utils.h"

#include <signal.h>
#include <stdint.h>
#include <time.h>

#include <sys/types.h>

#include <openssl/crypto.h>

struct Server {
  int eventfd;
  int sockfd;
  SSL_CTX *ctx;
  Config *conf;
  time_t start_at;
  uint32_t age;
  sig_atomic_t closed;
  struct Command *commands;
};

void terminate_connection(Client *client);
off_t *get_authorization_end_at();
void get_server_time(time_t *server_start_at, uint32_t *server_age);
void handle_events(struct Server *server);
void start_server(Config *config);
Config *get_server_config();

string_t write_value(void *value, const enum TellyTypes type, const enum ProtocolVersion protover, char *buffer);
