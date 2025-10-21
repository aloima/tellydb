#pragma once

#include "server/server.h"

#include <stdint.h>

struct TransactionVariables get_transaction_variables();
void initialize_transactions();
void execute_transaction_block(struct TransactionBlock *block, struct Client *client);
