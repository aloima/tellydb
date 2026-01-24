#pragma once

#include "../database/database.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdalign.h>
#include <time.h>

#include <openssl/crypto.h>

#define RESP_BUF_SIZE 4096

struct TransactionBlock;
typedef struct TransactionBlock TransactionBlock;

struct Password;
typedef struct Password Password;

enum ProtocolVersion : uint8_t {
  RESP2 = 2,
  RESP3 = 3
};

enum ClientState : uint8_t {
  CLIENT_STATE_ACTIVE,
  CLIENT_STATE_PASSIVE,
  CLIENT_STATE_EMPTY
};

typedef struct {
  _Atomic(struct Command *) data;
  _Atomic(struct Subcommand *) subcommand;
  _Atomic(uint64_t) idx;
} UsedCommand;

typedef struct {
  alignas(64) _Atomic enum ClientState state;
  int id, connfd;
  SSL *ssl;

  time_t connected_at;
  Database *database;
  UsedCommand *command;
  char *lib_name, *lib_ver;

  Password *password;
  enum ProtocolVersion protover;

  bool locked;
  TransactionBlock *waiting_block;

  char *write_buf;
} Client;

int initialize_clients();
void free_clients();

Client *get_client(const uint32_t id);

uint32_t get_last_connection_client_id();
uint16_t get_client_count();

Client *add_client(const int connfd);
bool remove_client(const int id);
