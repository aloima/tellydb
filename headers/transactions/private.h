#pragma once

#include "server/server.h"

#include <stdint.h>

struct MultipleTransactions {
  struct Transaction *transactions;
  uint64_t transaction_count;
};

struct TransactionVariables *get_transaction_variables();
void initialize_transactions();
void execute_transaction_block(struct TransactionBlock *block);
