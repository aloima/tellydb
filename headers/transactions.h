#pragma once

#include "commands/_commands.h"
#include "database/database.h"
#include "server/server.h"
#include "config.h"
#include "resp.h"
#include "auth.h"

#include <stdbool.h>
#include <stdint.h>

#define MAX_RESPONSE_SIZE 262144

struct Transaction {
  commanddata_t data;
  struct Command *command;
  struct Database *database;
};

struct TransactionBlock {
  struct Client *client;
  struct Password *password;
  struct Transaction *transactions;
  uint64_t transaction_count;
};

void create_transaction_thread(struct Configuration *config);
void deactive_transaction_thread();

uint64_t get_processed_transaction_count();
uint32_t get_transaction_count();
void release_queued_transaction_block(struct Client *client);
bool add_transaction(struct Client *client, const uint64_t command_idx, commanddata_t data);
struct TransactionBlock *reserve_transaction_block(struct Client *client, const bool as_queued);
void remove_transaction_block(struct TransactionBlock *block, const bool processed);
void free_transactions();

void execute_transaction_block(struct TransactionBlock *block);
