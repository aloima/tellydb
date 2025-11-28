#include <telly.h>

#include "../headers/transactions/private.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>

static struct TransactionVariables *variables;
static uint64_t processed_transaction_count = 0;

// Private method, accessed by create_transaction_thread method once.
void initialize_transactions() {
  variables = get_transaction_variables();
}

uint32_t get_transaction_count() {
  return estimate_tqueue_size(variables->queue);
}

uint64_t get_processed_transaction_count() {
  return processed_transaction_count;
}

struct TransactionBlock *add_transaction_block(struct TransactionBlock *block) {
  return push_tqueue(variables->queue, block);
}

static inline void prepare_transaction(
  struct Transaction *transaction, struct Client *client, const uint64_t command_idx, commanddata_t *data
) {
  transaction->command = &variables->commands[command_idx];
  transaction->data = *data;
  transaction->database = client->database;
}

bool add_transaction(struct Client *client, const uint64_t command_idx, commanddata_t *data) {
  if (client->waiting_block == NULL || IS_RELATED_TO_WAITING_TX(command_idx)) {
    struct TransactionBlock block;
    block.client = client;
    block.password = client->password;
    block.transaction_count = 1;
    block.transactions = malloc(sizeof(struct Transaction));
    block.waiting = false;

    prepare_transaction(&block.transactions[0], client, command_idx, data);
    push_tqueue(variables->queue, &block);

    if (estimate_tqueue_size(variables->queue) - atomic_load_explicit(&variables->waiting_count, memory_order_relaxed) == 1) {
      pthread_mutex_lock(&variables->mutex);
      pthread_cond_signal(&variables->cond);
      pthread_mutex_unlock(&variables->mutex);
    }
  } else {
    struct TransactionBlock *block = client->waiting_block;
    block->transaction_count += 1;
    block->transactions = realloc(block->transactions, sizeof(struct Transaction) * block->transaction_count);
    prepare_transaction(&block->transactions[block->transaction_count - 1], client, command_idx, data);
  }

  return true;
}

void remove_transaction_block(struct TransactionBlock *block, const bool processed) {
  if (processed) {
    processed_transaction_count += block->transaction_count;
  }

  for (uint64_t i = 0; i < block->transaction_count; ++i) {
    struct Transaction transaction = block->transactions[i];
    free_command_data(transaction.data);
  }

  free(block->transactions);
  block->client = NULL;
  block->password = NULL;
  block->transactions = NULL;
  block->transaction_count = 0;
  block->waiting = false;
}

void free_transactions() {
  struct ThreadQueue *queue = variables->queue;

  while (estimate_tqueue_size(queue) != 0) {
    struct TransactionBlock block;
    pop_tqueue(queue, &block);

    for (uint32_t i = 0; i < block.transaction_count; ++i) {
      struct Transaction transaction = block.transactions[i];
      free_command_data(transaction.data);
    }
  }

  free_tqueue(queue);
}

void execute_transaction_block(struct TransactionBlock *block, struct Client *client) {
  __builtin_prefetch(block->transactions, 0, 3);
  struct Password *password = block->password;

  if (block->transaction_count == 1) {
    struct Transaction *transaction = &block->transactions[0];
    struct Command *command = transaction->command;

    if ((password->permissions & command->permissions) != command->permissions) {
      WRITE_ERROR_MESSAGE(client, "No permissions to execute this command");
      return;
    }

    struct CommandEntry entry = CREATE_COMMAND_ENTRY(client, &transaction->data, transaction->database, password, variables->buffer);
    const string_t response = command->run(&entry);

    if (response.len != 0) {
      _write(client, response.value, response.len);
    }

    return;
  }

  const uint32_t transaction_count = block->transaction_count;
  string_t results[transaction_count];
  uint64_t result_count = 0;
  uint64_t length = 0;

  for (uint32_t i = 0; i < transaction_count; ++i) {
    struct Transaction transaction = block->transactions[i];
    struct Command *command = transaction.command;
    struct CommandEntry entry = CREATE_COMMAND_ENTRY(client, &transaction.data, transaction.database, password, variables->buffer);

    if ((password->permissions & command->permissions) != command->permissions) {
      WRITE_ERROR_MESSAGE(client, "No permissions to execute this command");
      return;
    }

    const string_t result = command->run(&entry);

    if (result.len == 0) {
      continue;
    }

    string_t *area = &results[result_count++];
    area->value = malloc(result.len);
    area->len = result.len;
    memcpy(area->value, result.value, area->len);

    length += area->len;
  }

  size_t at = get_digit_count(result_count) + 3;
  length += at;

  char *response = malloc(length + 1);
  sprintf(response, "*%" PRIu64 "\r\n", result_count);

  for (uint64_t i = 0; i < result_count; ++i) {
    string_t result = results[i];
    memcpy(response + at, result.value, result.len);
    at += result.len;
    free(result.value);
  }

  _write(client, response, length);
  free(response);
}
