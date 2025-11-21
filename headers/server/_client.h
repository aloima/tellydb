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
#define RESP_BUF_SIZE 4096

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

  char read_buf[RESP_BUF_SIZE];
};

bool initialize_client_maps();
void free_client_maps();

struct Client *get_client(const uint32_t id);

uint32_t get_last_connection_client_id();
struct Client *get_clients();
uint16_t get_client_count();

struct Client *add_client(const int connfd);
bool remove_client(const int id);
