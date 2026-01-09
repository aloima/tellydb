#pragma once

#include <telly.h>

#include <stdbool.h>

#include <semaphore.h>

extern ThreadQueue *tx_queue; // Stores transactions, destroyed by free_transaction_blocks() method, not by thread
extern sem_t *tx_sem; // Stores transaction count
extern _Atomic(bool) tx_thread_sleeping; // Determines transaction thread sleeping or not

void execute_transaction_block(TransactionBlock *block);
