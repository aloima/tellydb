#include <telly.h>

#include "../headers/transactions/private.h"

#include <signal.h>
#include <stdatomic.h>

#include <pthread.h>
#include <unistd.h>

static struct Configuration *conf;
static pthread_t thread;
static struct TransactionVariables variables;

static inline uint32_t calculate_block_count(const uint32_t current_at, const uint32_t current_end) {
  if (current_end >= current_at) {
    return (current_end - current_at);
  } else {
    return (current_end + (conf->max_transaction_blocks - current_at));
  }
}

void *transaction_thread(void *arg) {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  while (true) {
    const uint32_t current_at = atomic_load_explicit(variables.at, memory_order_acquire);
    const uint32_t current_end = atomic_load_explicit(variables.end, memory_order_acquire);
    const uint32_t current_waiting_blocks = atomic_load_explicit(variables.waiting_blocks, memory_order_relaxed);

    if (calculate_block_count(current_at, current_end) == current_waiting_blocks) {
      usleep(0);
      continue;
    }

    struct TransactionBlock *block = &(*variables.blocks)[current_at];
    struct Client *client;

    if (block->transaction_count == 0) {
      const uint32_t next_at = ((current_at + 1) % conf->max_transaction_blocks);
      atomic_store_explicit(variables.at, next_at, memory_order_release);
      continue;
    }

    if (current_waiting_blocks == 0) {
      client = get_client_from_id(block->client_id);
      execute_transaction_block(block, client);
      remove_transaction_block(block, true);

      const uint32_t next_at = ((current_at + 1) % conf->max_transaction_blocks);
      atomic_store_explicit(variables.at, next_at, memory_order_release);
      continue;
    }

    uint32_t _at = ((current_at + 1) % conf->max_transaction_blocks);
    block = &(*variables.blocks)[_at];
    client = get_client_from_id(block->client_id);

    // (if block does not have client but transactions, it needs to be passed) || (if block is queued/waiting block)
    while (!client || client->waiting_block) {
      if (block->transaction_count != 0) {
        const char *name = block->transactions[0].command->name;

        if (streq(name, "EXEC") || streq(name, "DISCARD") || streq(name, "MULTI")) {
          break;
        }
      }

      _at = (_at + 1) % conf->max_transaction_blocks;
      block = &(*variables.blocks)[_at];
      client = get_client_from_id(block->client_id);
    }

    execute_transaction_block(block, client);
    remove_transaction_block(block, true);
  }

  return NULL;
}

// Accessed by process
void deactive_transaction_thread() {
  pthread_cancel(thread);
  free(*variables.buffer);
}

// Accessed by process
void create_transaction_thread() {
  conf = get_server_configuration();
  variables = get_transaction_variables();
  initialize_transactions();

  *variables.commands = get_commands();
  *variables.blocks = calloc(conf->max_transaction_blocks, sizeof(struct TransactionBlock));
  *variables.buffer = malloc(MAX_RESPONSE_SIZE);

  pthread_create(&thread, NULL, transaction_thread, NULL);
  pthread_detach(thread);
}
