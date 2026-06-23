#include <telly.h>
#include "transactions.h"

static uint64_t processed_transaction_count = 0;
static uint32_t database_operations = 0;

uint32_t get_transaction_count() {
  return estimate_tqueue_size(tx_queue);
}

uint64_t get_processed_transaction_count() {
  return processed_transaction_count;
}

TransactionBlock *enqueue_to_transaction_queue(TransactionBlock **block) {
  TransactionBlock *res = push_tqueue(tx_queue, block);
  if (res == NULL)
    return NULL;

  signal_notifier(tx_notifier, 1);
  return res;
}

static inline void prepare_transaction(Transaction *transaction, Client *client, const UsedCommand *command, commanddata_t *data) {
  transaction->command = command->data;
  transaction->args = data->args;
  transaction->database = client->database;
  transaction->read_buf = client->read_buf;
  atomic_fetch_add_explicit(&transaction->read_buf->refcount, 1, memory_order_relaxed);
}

bool add_transaction(Client *client, const UsedCommand *command, commanddata_t *data) {
  if (client->waiting_block == NULL || server->commands[command->idx].flags.bits.waiting_tx) {
    TransactionBlock *block = malloc(sizeof(TransactionBlock));
    if (block == NULL)
      return false;

    block->type = TX_DIRECT;
    block->client = client;
    block->password = client->password;
    block->data.transaction = malloc(sizeof(Transaction));

    if (block->data.transaction == NULL) {
      free(block);
      return false;
    }

    prepare_transaction(block->data.transaction, client, command, data);
    while (push_tqueue(tx_queue, &block) == NULL)
      cpu_relax();

    signal_notifier(tx_notifier, 1);
  } else {
    MultipleTransactions *multiple = &client->waiting_block->data.multiple;
    multiple->transaction_count += 1;

    Transaction *transactions = NULL;

    if (multiple->transaction_count == 1) {
      transactions = malloc(sizeof(Transaction));
    } else {
      transactions = realloc(multiple->transactions, sizeof(Transaction) * multiple->transaction_count);
    }

    if (transactions == NULL) {
      multiple->transaction_count -= 1;
      return false;
    }

    multiple->transactions = transactions;
    prepare_transaction(&multiple->transactions[multiple->transaction_count - 1], client, command, data);
  }

  return true;
}

void remove_transaction_block(TransactionBlock *block) {
  switch (block->type) {
    case TX_DIRECT: {
      Transaction *transaction = block->data.transaction;
      processed_transaction_count += 1;

      if (transaction->read_buf) {
        if (atomic_fetch_sub_explicit(&transaction->read_buf->refcount, 1, memory_order_relaxed) == 1) {
          free(transaction->read_buf->data);
          free(transaction->read_buf);
        }
      }

      if (transaction->args.data != NULL)
        free(transaction->args.data);

      free(transaction);
      break;
    }

    case TX_WAITING: case TX_MULTIPLE: {
      MultipleTransactions multiple = block->data.multiple;
      if (block->type == TX_MULTIPLE) processed_transaction_count += multiple.transaction_count;

      for (uint32_t i = 0; i < multiple.transaction_count; ++i) {
        Transaction *tx = &multiple.transactions[i];

        if (tx->read_buf) {
          if (atomic_fetch_sub_explicit(&tx->read_buf->refcount, 1, memory_order_relaxed) == 1) {
            free(tx->read_buf->data);
            free(tx->read_buf);
          }
        }

        if (tx->args.data != NULL)
          free(tx->args.data);
      }

      free(multiple.transactions);
      break;
    }

    default:
      break;
  }

  free(block);
}

void free_transaction_blocks() {
  while (estimate_tqueue_size(tx_queue) != 0) {
    TransactionBlock *block;
    pop_tqueue(tx_queue, &block);
    remove_transaction_block(block);
  }

  free_tqueue(tx_queue);
}

static inline bool check_kv_expiries(void *element, void *external) {
  ASSERT(element, !=, NULL);
  ASSERT(external, !=, NULL);

  Database *database = (Database *) external;
  const string_t *key = (string_t *) element;
  KeyValue *kv = get_data(database, *key);
  if (kv == NULL)
    return true;

  const ExpiryState state = check_kv_expiry(database, kv);

  switch (state) {
    case EXPIRY_NOT_EXPIRED: case EXPIRY_EXPIRED:
      return true;

    case EXPIRY_SYSCALL_ERROR: case EXPIRY_DELETING_ERROR:
      return false;
  }

  unreachable();
}

static inline string_t execute_transaction(Client *client, struct Password *password, Transaction *transaction) {
  struct Command *command = transaction->command;
  struct CommandEntry entry = CREATE_COMMAND_ENTRY(client, &transaction->args, transaction->database, password);

  if ((password->permissions & command->permissions) != command->permissions) {
    WRITE_ERROR_MESSAGE(client, "No permissions to execute this command");
    return EMPTY_STRING();
  }

  if (command->flags.bits.access_database && (command->get_keys != NULL)) {
    command->get_keys(&entry);
    const uint64_t kv_count = server->keyspace->size.count;

    if (kv_count != 0) {
      int retry_count = 0;

      while(retry_count++ < EXPIRY_RETRY_COUNT) {
        // *server->keyspace[i] can be used, it belongs to transactions->args[i] and it is not disappeared yet.
        const bool status = foreach_vector(server->keyspace, check_kv_expiries, transaction->database);
        if (status)
          break;
      }

      clear_vector(server->keyspace, NULL);
    }
  }

  return command->run(&entry);
}

static inline void check_autosave(struct Command *command) {
  if (command->flags.bits.affect_database) {
    const time_t current_time = time(NULL);
    ASSERT(current_time, !=, INVALID_TIME);

    database_operations += 1;

    const uint32_t count = server->conf->autosave.count;
    const uint32_t seconds = server->conf->autosave.seconds;

    if (
      (database_operations >= count) &&
      (current_time < (tx_last_saved_at + seconds))
    ) {
      database_operations = 0;
      tx_last_saved_at = current_time;

      uint32_t server_age = server->age;
      server_age += difftime(current_time, server->start_at);

      const int saved = save_data(server_age);

      if (saved == 0) {
        const char *format = "More than %" PRIu32 " keys are changed in %" PRIu32 " seconds, auto-saved successfully.";
        write_log(LOG_INFO, format, count, seconds);
      } else if (saved == -1) {
        const char *format = "More than %" PRIu32 " keys are changed in %" PRIu32 " seconds, but cannot auto-saved.";
        write_log(LOG_INFO, format, count, seconds);
      }

      unreachable();
    } else if (current_time >= (tx_last_saved_at + seconds)) {
      database_operations = 1;
      tx_last_saved_at = current_time;
    }
  }
}

void execute_transaction_block(TransactionBlock *block) {
  Client *client = ((block->client->id != -1) ? block->client : NULL);
  struct Password *password = block->password;

  switch (block->type) {
    case TX_DIRECT: {
      struct Transaction *transaction = block->data.transaction;
      const string_t response = execute_transaction(client, password, transaction);
      check_autosave(transaction->command);

      if (response.len != 0) add_io_request(IOOP_WRITE, client, response);
      break;
    }

    case TX_MULTIPLE: {
      MultipleTransactions multiple = block->data.multiple;
      string_t results[multiple.transaction_count];
      uint64_t result_count = 0;
      uint64_t length = 0;

      for (uint32_t i = 0; i < multiple.transaction_count; ++i) {
        struct Transaction *transaction = &multiple.transactions[i];
        const string_t result = execute_transaction(client, password, transaction);
        check_autosave(transaction->command);
        if (result.len == 0) continue;

        string_t *area = &results[result_count++];
        area->value = malloc(result.len);
        area->len = result.len;
        ASSERT(memcpy(area->value, result.value, area->len), !=, NULL);

        length += area->len;
      }

      size_t at = get_digit_count(result_count) + 3;
      length += at;

      if (client != NULL && client->write_buf != NULL)
        sprintf(client->write_buf, "*%" PRIu64 "\r\n", result_count);

      for (uint64_t i = 0; i < result_count; ++i) {
        string_t result = results[i];
        ASSERT(memcpy(client->write_buf + at, result.value, result.len), !=, NULL);
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
