#pragma once

#include "database/database.h"
#include "server/server.h"
#include "config.h"
#include "resp.h"
#include "auth.h"

#include <stdbool.h>
#include <stdint.h>

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
bool add_transaction(struct Client *client, struct Command *command, commanddata_t data);
struct TransactionBlock *reserve_transaction_block();
void remove_transaction_block(struct TransactionBlock *block);
void free_transactions();

void execute_transaction_block(struct TransactionBlock *block);
