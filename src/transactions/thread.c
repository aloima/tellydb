#include <telly.h>

#include "../headers/transactions/private.h"

#include <stdlib.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>

#include <semaphore.h>
#include <pthread.h>

static pthread_t thread;

static sem_t kill_sem;
static bool kill_pending = false;

static TransactionVariables *variables;

void *transaction_thread(void *arg) {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  atomic_init(&variables->thread_sleeping, true);
  variables->sem = malloc(sizeof(sem_t));
  sem_init(variables->sem, 0, 0);
  sem_init(&kill_sem, 0, 0);

  while (true) {
    sem_wait(variables->sem);
    atomic_store_explicit(&variables->thread_sleeping, false, memory_order_release);

    TransactionBlock *block;

    while (pop_tqueue(variables->queue, &block)) {
      execute_transaction_block(block);
      remove_transaction_block(block);
    }

    // May be data race, does not matter. If there is, a transaction will be executed. It is acceptable behavior.
    if (kill_pending) break;
    atomic_store_explicit(&variables->thread_sleeping, true, memory_order_release);
  }

  sem_destroy(variables->sem);
  free(variables->sem);

  sem_post(&kill_sem);
  return NULL;
}

void destroy_transaction_thread() {
  kill_pending = true;
  sem_post(variables->sem); // run transaction loop once

  sem_wait(&kill_sem); // wait until killed
  sem_destroy(&kill_sem);
}

void create_transaction_thread() {
  variables = get_transaction_variables();
  initialize_transactions();

  Config *conf = get_server_config();
  variables->commands = get_commands();
  variables->queue = create_tqueue(conf->max_transaction_blocks, sizeof(TransactionBlock *), alignof(TransactionBlock *));
  if (variables->queue == NULL) return write_log(LOG_ERR, "Cannot allocate transaction blocks, out of memory.");

  pthread_create(&thread, NULL, transaction_thread, NULL);
  pthread_detach(thread);
}
