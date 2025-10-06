#include <telly.h>

// For EBUSY enum
#include <errno.h> // IWYU pragma: keep
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include <pthread.h>

struct TransactionBlock *blocks;
struct Command *commands;

uint64_t processed_transaction_count = 0;
uint32_t block_count = 0;
uint32_t at = 0;
uint32_t end = 0;
struct Configuration *conf;

pthread_t thread;
pthread_cond_t cond;
pthread_mutex_t mutex;
char *buffer;

void *transaction_thread(void *arg) {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  while (true) {
    pthread_mutex_lock(&mutex);

    while (block_count == 0) {
      pthread_cond_wait(&cond, &mutex);
    }

    struct TransactionBlock *block = &blocks[at];

    if (!block->client) {
      at += 1;
      pthread_mutex_unlock(&mutex);
      continue;
    }

    if (block->client->waiting_block) {
      uint32_t _at = ((at + 1) % conf->max_transaction_blocks);
      block = &blocks[_at];

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
    } else {
      at = ((at + 1) % conf->max_transaction_blocks);
    }

    execute_transaction_block(block);
    remove_transaction_block(block, true);

    pthread_mutex_unlock(&mutex);
  }

  return NULL;
}

uint32_t get_transaction_count() {
  uint64_t transaction_count = 0;
  uint32_t idx = at;

  for (uint32_t i = 0; i < block_count; ++i) {
    transaction_count += blocks[idx].transaction_count;
    idx = ((idx + 1) % conf->max_transaction_blocks);
  }

  return transaction_count;
}

uint64_t get_processed_transaction_count() {
  return processed_transaction_count;
}

void deactive_transaction_thread() {
  while (pthread_mutex_trylock(&mutex) != EBUSY) {
    pthread_mutex_unlock(&mutex);
    pthread_cancel(thread);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
    free(buffer);
    return;
  }
}

void create_transaction_thread(struct Configuration *config) {
  conf = config;
  commands = get_commands();
  blocks = calloc(conf->max_transaction_blocks, sizeof(struct TransactionBlock));
  buffer = malloc(MAX_RESPONSE_SIZE);

  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
  pthread_create(&thread, NULL, transaction_thread, NULL);
  pthread_detach(thread);
}

struct TransactionBlock *reserve_transaction_block(struct Client *client, bool as_queued) {
  if (block_count == conf->max_transaction_blocks) {
    return NULL;
  }

  struct TransactionBlock *block = &blocks[end];
  block->client = client;
  block->password = client->password;
  block->transactions = NULL;
  block->transaction_count = 0;

  if (!as_queued) {
    block_count += 1;
  }

  end = ((end + 1) % conf->max_transaction_blocks);
  return block;
}

void release_queued_transaction_block(struct Client *client) {
  client->waiting_block = NULL;
  block_count += 1;
}

bool add_transaction(struct Client *client, const uint64_t command_idx, commanddata_t data) {
  struct Transaction *transaction;
  const char *name = commands[command_idx].name;

  pthread_mutex_lock(&mutex);

  if (client->waiting_block == NULL || streq(name, "EXEC") || streq(name, "DISCARD") || streq(name, "MULTI")) {
    struct TransactionBlock *block = reserve_transaction_block(client, false);

    if (!block) {
      return false;
    }

    block->transaction_count = 1;
    block->transactions = malloc(sizeof(struct Transaction));
    transaction = &block->transactions[0];
  } else {
    puts("a");
    struct TransactionBlock *block = client->waiting_block;
    block->transaction_count += 1;
    block->transactions = realloc(block->transactions, sizeof(struct Transaction) * block->transaction_count);
    transaction = &block->transactions[block->transaction_count - 1];
  }

  __builtin_prefetch(transaction, 1, 3);
  transaction->command = &commands[command_idx];
  transaction->data = data;
  transaction->database = client->database;

  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);

  return true;
}

// Run by thread, no need mutex
void remove_transaction_block(struct TransactionBlock *block, const bool processed) {
  if (processed) {
    block_count -= 1;
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

void free_transactions() {
  uint32_t idx = at;

  for (uint32_t i = 0; i < block_count; ++i) {
    struct TransactionBlock block = blocks[idx];
    idx = ((idx + 1) % conf->max_transaction_blocks);

    for (uint32_t j = 0; j < block.transaction_count; ++j) {
      struct Transaction transaction = block.transactions[j];
      free_command_data(transaction.data);
    }
  }

  free(blocks);
}

void execute_transaction_block(struct TransactionBlock *block) {
  struct Client *client = block->client;
  struct Password *password = block->password;

  if (block->transaction_count == 1) {
    struct Transaction transaction = block->transactions[0];
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

  string_t results[block->transaction_count];
  uint64_t result_count = 0;
  uint64_t length = 0;

  for (uint32_t i = 0; i < block->transaction_count; ++i) {
    struct Transaction transaction = block->transactions[i];
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
