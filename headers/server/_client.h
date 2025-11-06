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
  int id;
  time_t connected_at;
  struct Database *database;
  struct Command *command;
  char *lib_name;
  char *lib_ver;

  struct Password *password;
  enum ProtocolVersion protover;

  bool locked;
  struct TransactionBlock *waiting_block;
};

bool initialize_client_maps();
void free_client_maps();

struct Client *get_client(const int input);
struct Client *get_client_from_id(const uint32_t id);

uint32_t get_last_connection_client_id();
struct Client *get_clients();
uint16_t get_client_count();

struct Client *add_client(const int connfd);
bool remove_client(const int connfd);
