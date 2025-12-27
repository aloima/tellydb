#include <telly.h>
#include "transactions.h"

#include <stdlib.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>

#include <semaphore.h>
#include <pthread.h>

static pthread_t thread;

static sem_t kill_sem;
static bool kill_pending = false;

void *transaction_thread(void *arg) {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  atomic_init(&tx_thread_sleeping, true);
  tx_sem = malloc(sizeof(sem_t));
  sem_init(tx_sem, 0, 0);
  sem_init(&kill_sem, 0, 0);

  while (true) {
    sem_wait(tx_sem);
    atomic_store_explicit(&tx_thread_sleeping, false, memory_order_release);

    TransactionBlock *block;

    while (pop_tqueue(tx_queue, &block)) {
      execute_transaction_block(block);
      remove_transaction_block(block);
    }

    // May be data race, does not matter. If there is, a transaction will be executed. It is acceptable behavior.
    if (kill_pending) break;
    atomic_store_explicit(&tx_thread_sleeping, true, memory_order_release);
  }

  sem_destroy(tx_sem);
  free(tx_sem);

  sem_post(&kill_sem);
  return NULL;
}

void destroy_transaction_thread() {
  kill_pending = true;
  sem_post(tx_sem); // run transaction loop once

  sem_wait(&kill_sem); // wait until killed
  sem_destroy(&kill_sem);
}

void create_transaction_thread() {
  tx_queue = create_tqueue(server->conf->max_transaction_blocks, sizeof(TransactionBlock *), alignof(TransactionBlock *));
  if (tx_queue == NULL) return write_log(LOG_ERR, "Cannot allocate transaction blocks, out of memory.");

  pthread_create(&thread, NULL, transaction_thread, NULL);
  pthread_detach(thread);
}
