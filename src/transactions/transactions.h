#pragma once

#include <telly.h>

#include <time.h>

extern ThreadQueue *tx_queue; // Stores transactions, destroyed by free_transaction_blocks() method, not by thread
extern event_notifier_t *tx_notifier; // Stores transaction count
extern time_t tx_last_saved_at;

void execute_transaction_block(TransactionBlock *block);
