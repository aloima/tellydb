#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <time.h>

#include <openssl/crypto.h>

struct TransactionBlock;
#define RESP_BUF_SIZE 4096

enum ProtocolVersion : uint8_t {
  RESP2 = 2,
  RESP3 = 3
};

enum ClientState : uint8_t {
  CLIENT_STATE_ACTIVE,
  CLIENT_STATE_PASSIVE,
  CLIENT_STATE_EMPTY
};

struct Client {
  _Atomic enum ClientState state;
  int id, connfd;
  SSL *ssl;

  time_t connected_at;
  struct Database *database;
  struct Command *command;
  char *lib_name, *lib_ver;

  struct Password *password;
  enum ProtocolVersion protover;

  bool locked;
  struct TransactionBlock *waiting_block;

  char read_buf[RESP_BUF_SIZE];
};

bool initialize_clients();
void free_clients();

struct Client *get_client(const uint32_t id);

uint32_t get_last_connection_client_id();
struct Client *get_clients();
uint16_t get_client_count();

struct Client *add_client(const int connfd);
bool remove_client(const int id);
