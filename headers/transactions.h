#pragma once

#include "commands/data.h"
#include "database/database.h"
#include "server/server.h"
#include "resp.h"
#include "auth.h"

#include <stdbool.h>
#include <stdint.h>

#include <pthread.h>

#define MAX_RESPONSE_SIZE 262144

#define IS_RELATED_TO_WAITING_TX(commands, command_idx) ((commands)[command_idx].flags & CMD_FLAG_WAITING_TX)

enum TransactionBlockType : uint8_t {
  TX_UNINITIALIZED,
  TX_DIRECT,
  TX_WAITING,
  TX_MULTIPLE
};

typedef struct Transaction {
  commandargs_t args;
  struct Command *command;
  struct Database *database;
} Transaction;

typedef struct {
  Transaction *transactions;
  uint64_t transaction_count;
} MultipleTransactions;

typedef struct TransactionBlock {
  enum TransactionBlockType type;
  Client *client;
  struct Password *password;

  union {
    Transaction *transaction;
    MultipleTransactions multiple;
  } data;
} TransactionBlock;

int create_transaction_thread();
void destroy_transaction_thread();

uint64_t get_processed_transaction_count();
uint32_t get_transaction_count();

TransactionBlock *enqueue_to_transaction_queue(TransactionBlock **block);

bool add_transaction(Client *client, const uint64_t command_idx, commanddata_t *data);
void remove_transaction_block(TransactionBlock *block);

void free_transaction_blocks();
