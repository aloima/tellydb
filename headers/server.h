// Includes client and server structures and their methods

#pragma once

#include "config.h"
#include "utils.h"

#include <openssl/ssl.h>

#include <stdint.h>
#include <stdbool.h>
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

#define WRITE_OK(client) _write((client), "+OK\r\n", 5)
#define WRITE_ERROR(client) _write((client), "-ERROR\r\n", 8)


/* CLIENT */
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

  struct Password *password;
  enum ProtocolVersion protover;

  bool locked;
};

struct LinkedListNode *get_client_nodes();
struct Client *get_client(const int input);
struct Client *get_first_client();
struct Client *get_client_from_id(const uint32_t id);

uint32_t get_last_connection_client_id();
uint32_t get_client_count();

struct Client *add_client(const int connfd);
void remove_client(const int connfd);
void remove_first_client();
/* /CLIENT */



/* SERVER */
void terminate_connection(const int connfd);
off_t *get_authorization_end_at();
void get_server_time(time_t *server_start_at, uint64_t *server_age);
void start_server(struct Configuration *config);
struct Configuration *get_server_configuration();

ssize_t _read(struct Client *client, void *buf, const size_t nbytes);
ssize_t _write(struct Client *client, void *buf, const size_t nbytes);
void write_value(struct Client *client, void *value, enum TellyTypes type);
/* /SERVER */
