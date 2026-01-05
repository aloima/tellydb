#include <telly.h>
#include "transactions.h"

#include <stdlib.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <errno.h> // IWYU pragma: export

#include <semaphore.h>
#include <pthread.h>

static pthread_t thread;
static sem_t thread_sem; // Controls waits until thread opened, error situation does not matter
static bool failed = false; // Represents thread is not opened successfuly

static sem_t kill_sem;
static bool kill_pending = false;

void *transaction_thread(void *arg) {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  tx_sem = malloc(sizeof(sem_t));
  if (tx_sem == NULL) {
    failed = true;
    sem_post(&thread_sem);

    write_log(LOG_ERR, "Cannot allocate transaction semaphore, out of memory.");
    return NULL;
  }

  sem_init(tx_sem, 0, 0);
  sem_init(&kill_sem, 0, 0);
  atomic_init(&tx_thread_sleeping, true);
  sem_post(&thread_sem);

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

int create_transaction_thread() {
  tx_queue = create_tqueue(server->conf->max_transaction_blocks, sizeof(TransactionBlock *), alignof(TransactionBlock *));
  if (tx_queue == NULL) {
    write_log(LOG_ERR, "Cannot allocate transaction blocks, out of memory.");
    return -1;
  }

  const int code = pthread_create(&thread, NULL, transaction_thread, NULL);

  switch (code) {
    case EAGAIN:
      write_log(LOG_ERR, "Cannot create transaction thread, out of memory or thread limit problem of OS.");
      return -1;

    default:
      break;
  }

  sem_init(&thread_sem, 0, 0);
  pthread_detach(thread); // Thread is guaranteed joinable and available, so no need for control.

  sem_wait(&thread_sem);
  sem_destroy(&thread_sem);
  
  if (failed) return -1;
  else return 0;
}
