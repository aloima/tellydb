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

static sem_t thread_sem; // Controls initalization of thread, signalled if thread initialized successfully or not
static bool failed = false; // Represents thread is not opened successfuly

static _Atomic(bool) kill_pending; // Executes killing operation of thread
static sem_t kill_sem; // Waits until killing thread after kill_pending signal

void *transaction_thread(void *arg) {
  sigset_t *set = (sigset_t *) arg;
  pthread_sigmask(SIG_BLOCK, set, NULL);

  tx_sem = malloc(sizeof(sem_t));
  if (tx_sem == NULL) {
    write_log(LOG_ERR, "Cannot allocate transaction semaphore, out of memory.");

    failed = true;
    sem_post(&thread_sem);
    return NULL;
  }

  tx_queue = create_tqueue(server->conf->max_transaction_blocks, sizeof(TransactionBlock *), alignof(TransactionBlock *));
  if (tx_queue == NULL) {
    write_log(LOG_ERR, "Cannot allocate transaction blocks, out of memory.");

    failed = true;
    sem_post(&thread_sem);
    return NULL;
  }

  sem_init(tx_sem, 0, 0);
  sem_init(&kill_sem, 0, 0);
  atomic_init(&kill_pending, false);
  atomic_init(&tx_thread_sleeping, true);
  sem_post(&thread_sem);

  while (!atomic_load_explicit(&kill_pending, memory_order_relaxed)) {
    sem_wait(tx_sem);
    atomic_store_explicit(&tx_thread_sleeping, false, memory_order_release);

    TransactionBlock *block;

    while (pop_tqueue(tx_queue, &block)) {
      execute_transaction_block(block);
      remove_transaction_block(block);
    }

    atomic_store_explicit(&tx_thread_sleeping, true, memory_order_release);
  }

  sem_destroy(tx_sem);
  free(tx_sem);

  sem_post(&kill_sem);
  return NULL;
}

void destroy_transaction_thread() {
  atomic_store_explicit(&kill_pending, true, memory_order_relaxed);
  sem_post(tx_sem); // run transaction loop once

  sem_wait(&kill_sem);
  sem_destroy(&kill_sem);
}

int create_transaction_thread() {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);

  const int code = pthread_create(&thread, NULL, transaction_thread, &set);

  switch (code) {
    case EAGAIN:
      write_log(LOG_ERR, "Cannot create transaction thread, out of memory or thread limit problem of OS.");
      return -1;

    default:
      break;
  }

  sem_init(&thread_sem, 0, 0);
  pthread_detach(thread); // Thread is guaranteed joinable and available, so no need to control.

  sem_wait(&thread_sem);
  sem_destroy(&thread_sem);
  
  if (failed) return -1;
  else return 0;
}
