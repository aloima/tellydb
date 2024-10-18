#pragma once

#include "config.h"
#include "utils.h"

#include <openssl/ssl.h>

#include <stdint.h>
#include <time.h>

struct Client {
  SSL *ssl;
  int connfd;
  uint32_t id;
  time_t connected_at;
  struct Command *command;
  char *lib_name;
  char *lib_ver;
};

void start_server(struct Configuration *config);
struct Configuration *get_server_configuration();

struct Client **get_clients();
struct Client *get_client(const int input);

uint32_t get_last_connection_client_id();
uint32_t get_client_count();

struct Client *add_client(const int connfd, const uint32_t max_clients);
void remove_client(const int connfd);

ssize_t _read(struct Client *client, void *buf, const size_t nbytes);
ssize_t _write(struct Client *client, void *buf, const size_t nbytes);
void write_value(struct Client *client, value_t value, enum TellyTypes type);

/* RESP */
#define RDT_SSTRING '+'
#define RDT_BSTRING '$'
#define RDT_ARRAY '*'
#define RDT_INTEGER ':'
#define RDT_ERR '-'
#define RDT_CLOSE 0

typedef struct RESPData {
  uint8_t type;

  union {
    string_t string;
    bool boolean;
    int32_t integer;
    double doubl;
    struct RESPData **array;
  } value;

  uint32_t count;
} respdata_t;

respdata_t *get_resp_data(struct Client *client);
void free_resp_data(respdata_t *data);
/* /RESP */
