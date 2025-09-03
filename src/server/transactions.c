#include <telly.h>

// For EBUSY enum
#include <errno.h> // IWYU pragma: keep
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include <pthread.h>

struct TransactionBlock *blocks;

uint64_t processed_transaction_count = 0;
uint32_t block_count = 0;
uint32_t at = 0;
uint32_t end = 0;
struct Configuration *conf;

pthread_t thread;
pthread_cond_t cond;
pthread_mutex_t mutex;

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

    if (block && !block->client->waiting_block) {
      execute_transaction_block(block);
      remove_transaction_block(block);
    }

    at = ((at + 1) % conf->max_transaction_blocks);
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
    return;
  }
}

void create_transaction_thread(struct Configuration *config) {
  conf = config;
  blocks = calloc(conf->max_transaction_blocks, sizeof(struct TransactionBlock));

  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
  pthread_create(&thread, NULL, transaction_thread, NULL);
  pthread_detach(thread);
}

struct TransactionBlock *reserve_transaction_block() {
  if (!IS_IN_PROCESS()) {
    pthread_mutex_lock(&mutex);
  }

  if (block_count == conf->max_transaction_blocks) {
    return NULL;
  }

  struct TransactionBlock *block = &blocks[end];
  block_count += 1;
  end = ((end + 1) % conf->max_transaction_blocks);

  if (!IS_IN_PROCESS()) {
    pthread_mutex_unlock(&mutex);
  }

  return block;
}

bool add_transaction(struct Client *client, struct Command *command, commanddata_t data) {
  struct Transaction *transaction;
  pthread_mutex_lock(&mutex);

  if (client->waiting_block == NULL) {
    struct TransactionBlock *block = reserve_transaction_block();

    if (!block) {
      return false;
    }

    block->client = client;
    block->password = client->password;

    block->transaction_count += 1;
    block->transactions = malloc(sizeof(struct Transaction));
    transaction = &block->transactions[0];
  } else {
    struct TransactionBlock *block = client->waiting_block;
    block->transaction_count += 1;
    block->transactions = realloc(block->transactions, sizeof(struct Transaction) * block->transaction_count);
    transaction = &block->transactions[block->transaction_count - 1];
  }

  transaction->command = command;
  transaction->data = data;
  transaction->database = client->database;

  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);

  return true;
}

// Run by thread, no need mutex
void remove_transaction_block(struct TransactionBlock *block) {
  block_count -= 1;
  processed_transaction_count += block->transaction_count;

  for (uint32_t i = 0; i < block->transaction_count; ++i) {
    struct Transaction transaction = block->transactions[i];

    if (transaction.command) {
      free_command_data(transaction.data);
    }
  }

  free(block->transactions);
  block->transactions = NULL;
  block->transaction_count = 0;
}

void free_transactions() {
  uint32_t idx = at;

  for (uint32_t i = 0; i < block_count; ++i) {
    struct TransactionBlock block = blocks[idx];
    idx = ((idx + 1) % conf->max_transaction_blocks);

    for (uint32_t j = 0; i < block.transaction_count; ++j) {
      struct Transaction transaction = block.transactions[j];

      if (transaction.command) {
        free_command_data(transaction.data);
      }
    }
  }

  free(blocks);
}

void execute_transaction_block(struct TransactionBlock *block) {
  struct Client *client = block->client;
  struct Password *password = block->password;

  for (uint32_t i = 0; i < block->transaction_count; ++i) {
    struct Transaction transaction = block->transactions[i];
    struct Command *command = transaction.command;

    struct CommandEntry entry = {
      .client = client,
      .data = &transaction.data,
      .database = transaction.database,
      .password = password
    };

    if ((password->permissions & command->permissions) != command->permissions) {
      WRITE_ERROR_MESSAGE(client, "No permissions to execute this command");
      return;
    }

    command->run(entry);
  }
}
