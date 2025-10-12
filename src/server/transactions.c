#include <telly.h>

// For EBUSY enum
#include <errno.h> // IWYU pragma: keep
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <stdatomic.h>

#include <pthread.h>
#include <unistd.h>

struct TransactionBlock *blocks;
struct Command *commands;

uint64_t processed_transaction_count = 0;
_Atomic uint32_t at = 0; // Accessed by reserve_transaction_block method.
_Atomic uint32_t end = 0; // Accessed by reserve_transaction_block method.
_Atomic uint32_t waiting_blocks = 0; // Accessed by reserve_transaction_block method.
struct Configuration *conf;

pthread_t thread;
char *buffer;

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
    const uint32_t current_at = atomic_load_explicit(&at, memory_order_acquire);
    const uint32_t current_end = atomic_load_explicit(&end, memory_order_acquire);
    const uint32_t current_waiting_blocks = atomic_load_explicit(&waiting_blocks, memory_order_acquire);

    if (calculate_block_count(current_at, current_end) == current_waiting_blocks) {
      usleep(1);
      continue;
    }

    struct TransactionBlock *block = &blocks[current_at];

    if (block->transaction_count == 0) {
      const uint32_t next_at = ((current_at + 1) % conf->max_transaction_blocks);
      atomic_store_explicit(&at, next_at, memory_order_release);
      continue;
    }

    if (current_waiting_blocks == 0) {
      execute_transaction_block(block);
      remove_transaction_block(block, true);

      const uint32_t next_at = ((current_at + 1) % conf->max_transaction_blocks);
      atomic_store_explicit(&at, next_at, memory_order_release);
      continue;
    }

    uint32_t _at = ((current_at + 1) % conf->max_transaction_blocks);
    block = &blocks[_at];

    // (if block does not have client but transactions, it needs to be passed) || (if block is queued/waiting block)
    while (!block->client || block->client->waiting_block) {
      if (block->transaction_count != 0) {
        const char *name = block->transactions[0].command->name;

        if (streq(name, "EXEC") || streq(name, "DISCARD") || streq(name, "MULTI")) {
          break;
        }
      }

      _at = (_at + 1) % conf->max_transaction_blocks;
      block = &blocks[_at];
    }

    execute_transaction_block(block);
    remove_transaction_block(block, true);
  }

  return NULL;
}

// Accessed by thread
uint32_t get_transaction_count() {
  const uint32_t current_end = atomic_load(&end);
  uint32_t idx = atomic_load(&at);
  uint32_t transaction_count = 0;

  while (idx != current_end) {
    transaction_count += blocks[idx].transaction_count;
    idx = ((idx + 1) % conf->max_transaction_blocks);
  }

  return transaction_count;
}

// Accessed by thread
uint64_t get_processed_transaction_count() {
  return processed_transaction_count;
}

// Accessed by process
void deactive_transaction_thread() {
  pthread_cancel(thread);
  free(buffer);
}

// Accessed by process
void create_transaction_thread(struct Configuration *config) {
  conf = config;
  commands = get_commands();
  blocks = calloc(conf->max_transaction_blocks, sizeof(struct TransactionBlock));
  buffer = malloc(MAX_RESPONSE_SIZE);

  pthread_create(&thread, NULL, transaction_thread, NULL);
  pthread_detach(thread);
}

// Accessed by thread and process
struct TransactionBlock *prereserve_transaction_block(struct Client *client, const bool as_queued) {
  const uint32_t current_at = atomic_load_explicit(&at, memory_order_acquire);
  const uint32_t current_end = atomic_load_explicit(&end, memory_order_acquire);
  const uint32_t next_end = ((current_end + 1) % conf->max_transaction_blocks);

  if (VERY_UNLIKELY(current_at == next_end)) {
    return NULL;
  }

  struct TransactionBlock *block = &blocks[current_end];
  block->client = client;
  block->password = client->password;
  block->transactions = NULL;
  block->transaction_count = 0;

  if (as_queued) {
    atomic_fetch_add_explicit(&waiting_blocks, 1, memory_order_relaxed);
  }

  return block;
}

void reserve_transaction_block() {
  const uint32_t current_end = atomic_load_explicit(&end, memory_order_acquire);
  const uint32_t next_end = ((current_end + 1) % conf->max_transaction_blocks);
  atomic_store_explicit(&end, next_end, memory_order_release);
}

// Accessed by thread
void release_queued_transaction_block(struct Client *client) {
  client->waiting_block = NULL;
  atomic_fetch_sub_explicit(&waiting_blocks, 1, memory_order_relaxed);
}

// Accessed by process
bool add_transaction(struct Client *client, const uint64_t command_idx, commanddata_t data) {
  struct Transaction *transaction;
  const char *name = commands[command_idx].name;

  if (client->waiting_block == NULL || streq(name, "EXEC") || streq(name, "DISCARD") || streq(name, "MULTI")) {
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
  transaction->command = &commands[command_idx];
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
  block->client = NULL;
  block->password = NULL;
  block->transactions = NULL;
  block->transaction_count = 0;
}

// Accessed by process
void free_transactions() {
  const uint32_t current_end = atomic_load(&end);
  uint32_t idx = atomic_load(&at);

  while (idx != current_end) {
    struct TransactionBlock block = blocks[idx];
    idx = ((idx + 1) % conf->max_transaction_blocks);

    for (uint32_t i = 0; i < block.transaction_count; ++i) {
      struct Transaction transaction = block.transactions[i];
      free_command_data(transaction.data);
    }
  }

  free(blocks);
}

// Accessed by thread
void execute_transaction_block(struct TransactionBlock *block) {
  struct Client *client = block->client;
  const struct Transaction *transactions = block->transactions;
  struct Password *password = block->password;

  if (block->transaction_count == 1) {
    struct Transaction transaction = transactions[0];
    struct Command *command = transaction.command;
    struct CommandEntry entry = CREATE_COMMAND_ENTRY(client, &transaction.data, transaction.database, password, buffer);

    if ((password->permissions & command->permissions) != command->permissions) {
      WRITE_ERROR_MESSAGE(client, "No permissions to execute this command");
      return;
    }

    string_t response = command->run(entry);

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
    struct CommandEntry entry = CREATE_COMMAND_ENTRY(client, &transaction.data, transaction.database, password, buffer);

    if ((password->permissions & command->permissions) != command->permissions) {
      WRITE_ERROR_MESSAGE(client, "No permissions to execute this command");
      return;
    }

    string_t result = command->run(entry);

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
