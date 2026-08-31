#pragma once

#include "commands/commands.h"
#include "database/database.h"
#include "server/server.h"
#include "resp/resp.h"

#include <stdint.h>

#define MAX_RESPONSE_SIZE 262144

typedef enum : uint8_t {
  TX_UNINITIALIZED,
  TX_DIRECT,
  TX_WAITING,
  TX_MULTIPLE
} TransactionBlockType;

typedef struct {
  commandargs_t args;
  Command *command;
  Database *database;
  QueryBuffer *read_buf;
} Transaction;

typedef struct {
  Transaction *transactions;
  uint64_t transaction_count;
} MultipleTransactions;

typedef struct TransactionBlock {
  TransactionBlockType type;
  Client *client;
  Password *password;

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

bool add_transaction(Client *client, const UsedCommand *command, commanddata_t *data);
void remove_transaction_block(TransactionBlock *block);

void free_transaction_blocks();
