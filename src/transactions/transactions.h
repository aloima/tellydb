#pragma once

#include <telly.h>

#include <stdbool.h>

#include <semaphore.h>

extern ThreadQueue *tx_queue;
extern sem_t *tx_sem;
extern _Atomic bool tx_thread_sleeping;

void execute_transaction_block(TransactionBlock *block);
