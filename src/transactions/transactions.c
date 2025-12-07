#include <telly.h>

#include "../headers/transactions/private.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>

static TransactionVariables *variables;
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

TransactionBlock *add_transaction_block(TransactionBlock *block) {
  return push_tqueue(variables->queue, block);
}

static inline void prepare_transaction(Transaction *transaction, Client *client, const uint64_t command_idx, commanddata_t *data) {
  transaction->command = &variables->commands[command_idx];
  transaction->data = *data;
  transaction->database = client->database;
}

bool add_transaction(Client *client, const uint64_t command_idx, commanddata_t *data) {
  TransactionBlock block;

  if (client->waiting_block == NULL || IS_RELATED_TO_WAITING_TX(variables->commands, command_idx)) {
    block.type = TX_DIRECT;

    block.client = client;
    block.password = client->password;
    block.data.transaction = malloc(sizeof(Transaction));

    prepare_transaction(block.data.transaction, client, command_idx, data);
    push_tqueue(variables->queue, &block);

    if (estimate_tqueue_size(variables->queue) - atomic_load_explicit(&variables->waiting_count, memory_order_relaxed) == 1) {
      pthread_mutex_lock(&variables->mutex);
      pthread_cond_signal(&variables->cond);
      pthread_mutex_unlock(&variables->mutex);
    }
  } else {
    block.type = TX_WAITING;

    MultipleTransactions *multiple = &block.data.multiple;
    multiple->transaction_count += 1;
    multiple->transactions = realloc(multiple->transactions, sizeof(Transaction) * multiple->transaction_count);
    prepare_transaction(&multiple->transactions[multiple->transaction_count - 1], client, command_idx, data);
  }

  return true;
}

void remove_transaction_block(TransactionBlock *block) {
  switch (block->type) {
    case TX_DIRECT: {
      Transaction *transaction = block->data.transaction;
      processed_transaction_count += 1;
      free(transaction);
      break;
    }

    case TX_WAITING: case TX_MULTIPLE: {
      MultipleTransactions multiple = block->data.multiple;
      if (block->type == TX_MULTIPLE) processed_transaction_count += multiple.transaction_count;

      free(multiple.transactions);
      break;
    };

    default:
      break;
  }

  block->type = TX_UNINITIALIZED;
}

void free_transaction_blocks() {
  struct ThreadQueue *queue = variables->queue;

  while (estimate_tqueue_size(queue) != 0) {
    TransactionBlock block;
    pop_tqueue(queue, &block);
    remove_transaction_block(&block);
  }

  free_tqueue(queue);
}

static inline string_t execute_transaction(Client *client, struct Password *password, Transaction *transaction) {
  struct Command *command = transaction->command;
  struct CommandEntry entry = CREATE_COMMAND_ENTRY(client, &transaction->data, transaction->database, password);

  if ((password->permissions & command->permissions) != command->permissions) {
    WRITE_ERROR_MESSAGE(client, "No permissions to execute this command");
    return EMPTY_STRING();
  }

  return command->run(&entry);
}

void execute_transaction_block(TransactionBlock *block) {
  Client *client = ((block->client->id != -1) ? block->client : NULL);
  struct Password *password = block->password;

  switch (block->type) {
    case TX_DIRECT: {
      const string_t response = execute_transaction(client, password, block->data.transaction);
      if (response.len != 0) add_io_request(IOOP_WRITE, client, response);
      break;
    }

    // TODO: client->write_buf collision
    case TX_MULTIPLE: {
      MultipleTransactions multiple = block->data.multiple;
      string_t results[multiple.transaction_count];
      uint64_t result_count = 0;
      uint64_t length = 0;

      for (uint32_t i = 0; i < multiple.transaction_count; ++i) {
        const string_t result = execute_transaction(client, password, &multiple.transactions[i]);
        if (result.len == 0) continue;

        string_t *area = &results[result_count++];
        area->value = malloc(result.len);
        area->len = result.len;
        memcpy(area->value, result.value, area->len);

        length += area->len;
      }

      size_t at = get_digit_count(result_count) + 3;
      length += at;

      sprintf(client->write_buf, "*%" PRIu64 "\r\n", result_count);

      for (uint64_t i = 0; i < result_count; ++i) {
        string_t result = results[i];
        memcpy(client->write_buf + at, result.value, result.len);
        at += result.len;
        free(result.value);
      }

      add_io_request(IOOP_WRITE, client, CREATE_STRING(client->write_buf, length));
      break;
    }

    default:
      break;
  }
}
