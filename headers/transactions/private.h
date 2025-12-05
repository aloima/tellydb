#pragma once

#include "server/server.h"

#include <stdint.h>

struct TransactionBlockStruct;
typedef struct TransactionBlockStruct TransactionBlock;

struct TransactionStruct;
typedef struct TransactionStruct Transaction;

typedef struct {
  Transaction *transactions;
  uint64_t transaction_count;
} MultipleTransactions;

typedef struct {
  struct ThreadQueue *queue;
  char *buffer;
  struct Command *commands;
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  _Atomic uint64_t waiting_count;
} TransactionVariables;

TransactionVariables *get_transaction_variables();
void initialize_transactions();
void execute_transaction_block(TransactionBlock *block);
