#include <telly.h>

#include "../headers/transactions/private.h"

#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>

#include <pthread.h>
#include <unistd.h>

static struct Configuration *conf;
static pthread_t thread;
static struct TransactionVariables variables;
static bool killed = false;

void *transaction_thread(void *arg) {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  while (!killed) {
    uint64_t idx = 0;
    struct TransactionBlock *block = get_tqueue_value(*variables.queue, idx);
    bool found = false;

    while (block != NULL) {
      found = true;

      if (block->client_id == -1) {
        pop_tqueue(*variables.queue);
        break;
      }

      if (!block->waiting) {
        struct Client *client = get_client_from_id(block->client_id);
        execute_transaction_block(block, client);
        remove_transaction_block(block, true);

        if (idx == 0) pop_tqueue(*variables.queue);
        break;
      }

      idx += 1;
      block = get_tqueue_value(*variables.queue, idx);
    }

    if (!found) {
      pthread_mutex_lock(variables.mutex);
      pthread_cond_wait(variables.cond, variables.mutex);
      pthread_mutex_unlock(variables.mutex);
    }
  }

  return NULL;
}

// Accessed by process
void deactive_transaction_thread() {
  killed = true;
  pthread_mutex_lock(variables.mutex);
  pthread_cond_signal(variables.cond);
  pthread_mutex_unlock(variables.mutex);
  usleep(5);

  pthread_cond_destroy(variables.cond);
  pthread_mutex_destroy(variables.mutex);
  pthread_cancel(thread);
  free(*variables.buffer);
}

// Accessed by process
void create_transaction_thread() {
  conf = get_server_configuration();
  variables = get_transaction_variables();
  initialize_transactions();

  *variables.commands = get_commands();
  *variables.queue = create_tqueue(conf->max_transaction_blocks, sizeof(struct TransactionBlock), _Alignof(struct TransactionBlock));
  if (*variables.queue == NULL) return write_log(LOG_ERR, "Cannot allocate transaction blocks, out of memory.");

  *variables.buffer = malloc(MAX_RESPONSE_SIZE);

  pthread_create(&thread, NULL, transaction_thread, NULL);
  pthread_detach(thread);
}
