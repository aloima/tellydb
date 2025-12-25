#pragma once

#include "server/server.h"

#include <stdint.h>

#include <semaphore.h>

struct TransactionBlock;
typedef struct TransactionBlock TransactionBlock;

struct Transaction;
typedef struct Transaction Transaction;

typedef struct {
  Transaction *transactions;
  uint64_t transaction_count;
} MultipleTransactions;

typedef struct {
  struct ThreadQueue *queue;
  struct Command *commands;
  sem_t *sem;
  _Atomic bool thread_sleeping;
} TransactionVariables;

TransactionVariables *get_transaction_variables();
void initialize_transactions();
void execute_transaction_block(TransactionBlock *block);
