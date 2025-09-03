#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include <openssl/crypto.h>

enum ProtocolVersion {
  RESP2 = 2,
  RESP3 = 3
};

struct TransactionBlock;

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
  bool waiting_execution;
  struct TransactionBlock *waiting_block;
};

struct Client *get_client(const int input);
struct LinkedListNode *get_head_client();
struct Client *get_client_from_id(const uint32_t id);

uint32_t get_last_connection_client_id();
uint32_t get_client_count();

struct Client *add_client(const int connfd);
void remove_client(const int connfd);
void remove_head_client();
