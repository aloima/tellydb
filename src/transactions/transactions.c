#include <telly.h>

#include "../headers/transactions/private.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>

static struct Configuration *conf;
static struct TransactionVariables variables;
static uint64_t processed_transaction_count = 0;

// Command indexes related to waiting blocks
static uint64_t block_idx[3];

// Private method, accessed by create_transaction_thread method once.
void initialize_transactions() {
  conf = get_server_configuration();
  variables = get_transaction_variables();
  block_idx[0] = get_command_index("EXEC", 4)->idx;
  block_idx[1] = get_command_index("DISCARD", 7)->idx;
  block_idx[2] = get_command_index("MULTI", 5)->idx;
}

// Accessed by thread
uint32_t get_transaction_count() {
  const uint32_t current_end = atomic_load(variables.end);
  uint32_t idx = atomic_load(variables.at);
  uint32_t transaction_count = 0;

  while (idx != current_end) {
    transaction_count += (*variables.blocks)[idx].transaction_count;
    idx = ((idx + 1) % conf->max_transaction_blocks);
  }

  return transaction_count;
}

// Accessed by thread
uint64_t get_processed_transaction_count() {
  return processed_transaction_count;
}

// Accessed by thread and process
struct TransactionBlock *prereserve_transaction_block(struct Client *client, const bool as_queued) {
  const uint32_t current_at = atomic_load_explicit(variables.at, memory_order_relaxed);
  const uint32_t current_end = atomic_load_explicit(variables.end, memory_order_relaxed);
  const uint32_t next_end = ((current_end + 1) % conf->max_transaction_blocks);

  if (VERY_UNLIKELY(current_at == next_end)) {
    return NULL;
  }

  struct TransactionBlock *block = &(*variables.blocks)[current_end];
  __builtin_prefetch(block, 1, 3);
  block->client_id = client->id;
  block->password = client->password;
  // Already zeroed
  // block->transactions = NULL;
  // block->transaction_count = 0;

  if (as_queued) {
    atomic_fetch_add_explicit(variables.waiting_blocks, 1, memory_order_relaxed);
  }

  return block;
}

// Accessed by thread and process.
void reserve_transaction_block() {
  const uint32_t current_end = atomic_load_explicit(variables.end, memory_order_relaxed);
  const uint32_t next_end = ((current_end + 1) % conf->max_transaction_blocks);
  atomic_store_explicit(variables.end, next_end, memory_order_release);
}

// Accessed by thread
void release_queued_transaction_block(struct Client *client) {
  client->waiting_block = NULL;
  atomic_fetch_sub_explicit(variables.waiting_blocks, 1, memory_order_relaxed);
}

// Accessed by process
bool add_transaction(struct Client *client, const uint64_t command_idx, commanddata_t data) {
  struct Transaction *transaction;

  if (client->waiting_block == NULL || command_idx == block_idx[0] || command_idx == block_idx[1] || command_idx == block_idx[2]) {
    struct TransactionBlock *block = prereserve_transaction_block(client, false);

    if (!block) {
      return false;
    }

    block->transaction_count = 1;
    block->transactions = malloc(sizeof(struct Transaction));
    transaction = &block->transactions[0];
  } else {
    struct TransactionBlock *block = client->waiting_block;
    block->transaction_count += 1;
    block->transactions = realloc(block->transactions, sizeof(struct Transaction) * block->transaction_count);
    transaction = &block->transactions[block->transaction_count - 1];
  }

  __builtin_prefetch(transaction, 1, 3);
  transaction->command = &(*variables.commands)[command_idx];
  transaction->data = data;
  transaction->database = client->database;

  reserve_transaction_block();
  return true;
}

// Accessed by thread
void remove_transaction_block(struct TransactionBlock *block, const bool processed) {
  if (processed) {
    processed_transaction_count += block->transaction_count;
  }

  for (uint64_t i = 0; i < block->transaction_count; ++i) {
    struct Transaction transaction = block->transactions[i];
    free_command_data(transaction.data);
  }

  free(block->transactions);
  block->client_id = -1;
  block->password = NULL;
  block->transactions = NULL;
  block->transaction_count = 0;
}

// Accessed by process
void free_transactions() {
  const uint32_t current_end = atomic_load(variables.end);
  uint32_t idx = atomic_load(variables.at);

  while (idx != current_end) {
    struct TransactionBlock block = (*variables.blocks)[idx];
    idx = ((idx + 1) % conf->max_transaction_blocks);

    for (uint32_t i = 0; i < block.transaction_count; ++i) {
      struct Transaction transaction = block.transactions[i];
      free_command_data(transaction.data);
    }
  }

  free(*variables.blocks);
}

// Accessed by thread
void execute_transaction_block(struct TransactionBlock *block, struct Client *client) {
  const struct Transaction *transactions = block->transactions;
  struct Password *password = block->password;

  if (block->transaction_count == 1) {
    struct Transaction transaction = transactions[0];
    struct Command *command = transaction.command;
    struct CommandEntry entry = CREATE_COMMAND_ENTRY(client, &transaction.data, transaction.database, password, *variables.buffer);

    if ((password->permissions & command->permissions) != command->permissions) {
      WRITE_ERROR_MESSAGE(client, "No permissions to execute this command");
      return;
    }

    const string_t response = command->run(entry);

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
    struct Transaction transaction = transactions[i];
    struct Command *command = transaction.command;
    struct CommandEntry entry = CREATE_COMMAND_ENTRY(client, &transaction.data, transaction.database, password, *variables.buffer);

    if ((password->permissions & command->permissions) != command->permissions) {
      WRITE_ERROR_MESSAGE(client, "No permissions to execute this command");
      return;
    }

    const string_t result = command->run(entry);

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
