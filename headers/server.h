// Includes client and server structures and their methods

#pragma once

#include "database.h"
#include "config.h"
#include "utils.h"

#include <openssl/crypto.h>
#include <openssl/ssl.h>

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>

#define _read(client, buf, nbytes) ((client)->ssl ? SSL_read((client)->ssl, (buf), (nbytes)) : read((client)->connfd, (buf), (nbytes)))
#define _write(client, buf, nbytes) ((client)->ssl ? SSL_write((client)->ssl, (buf), (nbytes)) : write((client)->connfd, (buf), (nbytes)))

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
  struct Database *database;
  struct Command *command;
  char *lib_name;
  char *lib_ver;

  struct Password *password;
  enum ProtocolVersion protover;

  bool locked;
};

struct Client *get_client(const int input);
struct LinkedListNode *get_head_client();
struct Client *get_client_from_id(const uint32_t id);

uint32_t get_last_connection_client_id();
uint32_t get_client_count();

struct Client *add_client(const int connfd);
void remove_client(const int connfd);
void remove_head_client();
/* /CLIENT */



/* SERVER */
void terminate_connection(const int connfd);
off_t *get_authorization_end_at();
void get_server_time(time_t *server_start_at, uint32_t *server_age);
void start_server(struct Configuration *config);
struct Configuration *get_server_configuration();

void write_value(struct Client *client, void *value, enum TellyTypes type);
/* /SERVER */
