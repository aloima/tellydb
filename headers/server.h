#pragma once

#include "config.h"
#include "utils.h"

#include <openssl/ssl.h>

#include <stdint.h>
#include <time.h>

#define WRITE_NULL_REPLY(client) \
  switch ((client)->protover) {\
    case RESP2:\
      _write((client), "$-1\r\n", 5);\
      break;\
\
    case RESP3:\
      _write((client), "_\r\n", 3);\
      break;\
  }

enum ProtocolVersion {
  RESP2 = 2,
  RESP3 = 3
};

struct Client {
  SSL *ssl;
  int connfd;
  uint32_t id;
  time_t connected_at;
  struct Command *command;
  char *lib_name;
  char *lib_ver;

  enum ProtocolVersion protover;
};

void get_server_time(time_t *server_start_at, uint64_t *server_age);
void start_server(struct Configuration *config);
struct Configuration *get_server_configuration();

struct Client **get_clients();
struct Client *get_client(const int input);

uint32_t get_last_connection_client_id();
uint32_t get_client_count();

struct Client *add_client(const int connfd);
void remove_client(const int connfd);

ssize_t _read(struct Client *client, void *buf, const size_t nbytes);
ssize_t _write(struct Client *client, void *buf, const size_t nbytes);
void write_value(struct Client *client, void *value, enum TellyTypes type);

/* RESP */
#define RDT_SSTRING '+'
#define RDT_BSTRING '$'
#define RDT_ARRAY '*'
#define RDT_INTEGER ':'
#define RDT_ERR '-'
#define RDT_CLOSE 0

typedef struct CommandData {
  bool close;
  string_t name;
  string_t *args;
  uint32_t arg_count;
} commanddata_t;

commanddata_t *get_command_data(struct Client *client);
void free_command_data(commanddata_t *data);
/* /RESP */
