#pragma once

#include "commands/_commands.h"
#include "database/database.h"
#include "server/server.h"
#include "resp.h"
#include "auth.h"

#include <stdbool.h>
#include <stdint.h>

#include <pthread.h>

#include "../headers/transactions/private.h"

#define MAX_RESPONSE_SIZE 262144

#define IS_RELATED_TO_WAITING_TX(command_idx) ({ \
  /* DISCARD || MULTI || EXEC */ \
  ((command_idx) == 9 || (command_idx) == 10 || (command_idx) == 13); \
})

enum TransactionBlockType : uint8_t {
  TX_UNINITIALIZED,
  TX_DIRECT,
  TX_WAITING,
  TX_MULTIPLE
};

struct Transaction {
  commanddata_t data;
  struct Command *command;
  struct Database *database;
};

struct TransactionBlock {
  enum TransactionBlockType type;
  struct Client *client;
  struct Password *password;

  union {
    struct Transaction *transaction;
    struct MultipleTransactions multiple;
  } data;
};

struct TransactionVariables {
  struct ThreadQueue *queue;
  char *buffer;
  struct Command *commands;
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  _Atomic uint64_t waiting_count;
};

void create_transaction_thread();
void deactive_transaction_thread();

uint64_t get_processed_transaction_count();
uint32_t get_transaction_count();

struct TransactionBlock *add_transaction_block(struct TransactionBlock *block);
bool add_transaction(struct Client *client, const uint64_t command_idx, commanddata_t *data);
void remove_transaction_block(struct TransactionBlock *block);

void free_transaction_blocks();
