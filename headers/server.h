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


/* AUTH */
enum Permissions {
  P_READ   = 0b00000001,
  P_WRITE  = 0b00000010,
  P_CLIENT = 0b00000100,
  P_CONFIG = 0b00000100,
  P_AUTH   = 0b00001000,
  P_SERVER = 0b00010000,
};

struct Password {
  unsigned char data[48];
  uint8_t permissions;
  uint8_t _padding[3];
};

bool create_constant_passwords();
void free_constant_passwords();
struct Password *get_full_password();
struct Password *get_empty_password();

bool initialize_kdf();
void free_kdf();

struct Password **get_passwords();
uint32_t get_password_count();
uint16_t get_authorization_from_file(const int fd, char *block, const uint16_t block_size);
void free_passwords();

void add_password(struct Client *client, const string_t data, const uint8_t permissions);
bool remove_password(struct Client *executor, char *value, const size_t value_len);
int32_t where_password(char *value, const size_t value_len);
struct Password *get_password(char *value, const size_t value_len);
bool edit_password(char *value, const size_t value_len, const uint32_t permissions);
/* /AUTH */


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


/* RESP */
#define RDT_SSTRING '+'
#define RDT_BSTRING '$'
#define RDT_ARRAY '*'
#define RDT_INTEGER ':'
#define RDT_ERR '-'
#define RDT_CLOSE 0

typedef struct CommandData {
  string_t name;
  string_t *args;
  uint32_t arg_count;
  bool close;
} commanddata_t;

commanddata_t *get_command_data(struct Client *client);
void free_command_data(commanddata_t *data);
/* /RESP */
