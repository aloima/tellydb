#include <telly.h>

#include "../headers/transactions/private.h"

#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>

#include <pthread.h>
#include <unistd.h>

static pthread_t thread;
static TransactionVariables *variables;
static _Atomic bool killed;

void *transaction_thread(void *arg) {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  while (!atomic_load_explicit(&killed, memory_order_acquire)) {
    bool found = false;
    TransactionBlock block;

    while (pop_tqueue(variables->queue, &block)) {
      found = true;

      if (block.type == TX_DIRECT) {
        Client *client;

        if (block.client->id != -1) {
          client = block.client;
          __builtin_prefetch(client, 0, 3);
        } else {
          client = NULL;
        }

        execute_transaction_block(&block);
        remove_transaction_block(&block);

        break;
      }
    }

    if (!found) {
      pthread_mutex_lock(&variables->mutex);
      pthread_cond_wait(&variables->cond, &variables->mutex);
      pthread_mutex_unlock(&variables->mutex);
    }
  }

  pthread_cond_destroy(&variables->cond);
  pthread_mutex_destroy(&variables->mutex);

  return NULL;
}

void deactive_transaction_thread() {
  atomic_store_explicit(&killed, true, memory_order_release);
  pthread_mutex_lock(&variables->mutex);
  pthread_cond_signal(&variables->cond);
  pthread_mutex_unlock(&variables->mutex);
}

void create_transaction_thread() {
  variables = get_transaction_variables();
  initialize_transactions();

  struct Configuration *conf = get_server_configuration();
  variables->commands = get_commands();
  variables->queue = create_tqueue(conf->max_transaction_blocks, sizeof(TransactionBlock), _Alignof(TransactionBlock));
  if (variables->queue == NULL) return write_log(LOG_ERR, "Cannot allocate transaction blocks, out of memory.");

  atomic_init(&variables->waiting_count, 0);

  pthread_cond_init(&variables->cond, NULL);
  pthread_mutex_init(&variables->mutex, NULL);
  atomic_init(&killed, false);

  pthread_create(&thread, NULL, transaction_thread, NULL);
  pthread_detach(thread);
}
