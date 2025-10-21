#pragma once

#include "commands/_commands.h"
#include "database/database.h"
#include "server/server.h"
#include "config.h"
#include "resp.h"
#include "auth.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_RESPONSE_SIZE 262144

struct Transaction {
  commanddata_t data;
  struct Command *command;
  struct Database *database;
};

struct TransactionBlock {
  int client_id;
  struct Password *password;
  struct Transaction *transactions;
  uint64_t transaction_count;
};

struct TransactionVariables {
  _Atomic uint32_t *at, *end;
  _Atomic uint32_t *waiting_blocks;
  char **buffer;
  struct TransactionBlock **blocks;
  struct Command **commands;
};

void create_transaction_thread();
void deactive_transaction_thread();

uint64_t get_processed_transaction_count();
uint32_t get_transaction_count();

bool add_transaction(struct Client *client, const uint64_t command_idx, commanddata_t data);
struct TransactionBlock *prereserve_transaction_block(struct Client *client, const bool as_queued);
void reserve_transaction_block();

void remove_transaction_block(struct TransactionBlock *block, const bool processed);
void release_queued_transaction_block(struct Client *client);

void free_transactions();
