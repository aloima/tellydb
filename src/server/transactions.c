#include "../../headers/telly.h"

#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>
#include <pthread.h>

struct Transaction **transactions = NULL;
uint32_t transaction_count = 0;
struct Configuration *conf = NULL;

void *transaction_thread(void *arg) {
  struct Configuration *conf = arg;

  while (true) {
    for (uint32_t i = 0; i < transaction_count; ++i) {
      struct Transaction *transaction = transactions[i];

      execute_command(transaction->client, transaction->command, conf);
      remove_transaction(transaction);
    }

    usleep(10);
  }

  return NULL;
}

uint32_t get_transaction_count() {
  return transaction_count;
}

pthread_t create_transaction_thread(struct Configuration *config) {
  conf = config;

  pthread_t thread;
  pthread_create(&thread, NULL, transaction_thread, conf);
  pthread_detach(thread);

  return thread;
}

void add_transaction(struct Client *client, respdata_t *data) {
  transaction_count += 1;

  if (transaction_count == 0) {
    transactions = malloc(sizeof(struct Transaction *));
  } else {
    transactions = realloc(transactions, transaction_count * sizeof(struct Transaction *));
  }

  const uint32_t id = transaction_count - 1;
  transactions[id] = malloc(sizeof(struct Transaction));
  transactions[id]->client = client;

  transactions[id]->command = malloc(sizeof(respdata_t));
  memcpy(transactions[id]->command, data, sizeof(respdata_t));
}

void remove_transaction(struct Transaction *transaction) {
  const uint64_t id = transaction - transactions[0];

  free(transaction->command);
  free(transaction);

  transaction_count -= 1;
  memcpy(transactions + id, transactions + id + 1, sizeof(struct Transaction *) * transaction_count);

  if (transaction_count == 0) {
    free(transactions);
    transactions = NULL;
  } else {
    transactions = realloc(transactions, sizeof(struct Transaction *) * transaction_count);
  }
}

void free_transactions() {
  for (uint32_t i = 0; i < transaction_count; ++i) {
    struct Transaction *transaction = transactions[i];

    free(transaction->command);
    free(transaction);
  }

  free(transactions);
}
