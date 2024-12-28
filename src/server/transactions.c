#include "../../headers/database.h"
#include "../../headers/commands.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>

#include <pthread.h>

struct Transaction **transactions;
uint32_t transaction_count = 0;
struct Configuration *conf;

pthread_t thread;
pthread_cond_t cond;
pthread_mutex_t mutex;
bool thread_loop = true;

void *transaction_thread() {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  pthread_mutex_lock(&mutex);

  while (thread_loop) {
    pthread_cond_wait(&cond, &mutex);

    struct Transaction *transaction = transactions[0];
    execute_command(transaction);
    remove_transaction(transaction);
  }

  pthread_mutex_unlock(&mutex);
  return NULL;
}

uint32_t get_transaction_count() {
  return transaction_count;
}

void deactive_transaction_thread() {
  thread_loop = false;

  while (pthread_mutex_trylock(&mutex) != EBUSY) {
    pthread_mutex_unlock(&mutex);
    pthread_cancel(thread);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
  }
}

void create_transaction_thread(struct Configuration *config) {
  conf = config;

  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
  pthread_create(&thread, NULL, transaction_thread, NULL);
  pthread_detach(thread);
}

void add_transaction(struct Client *client, commanddata_t *command) {
  pthread_mutex_lock(&mutex);
  transaction_count += 1;

  if (transaction_count == 1) {
    transactions = malloc(sizeof(struct Transaction *));
  } else {
    transactions = realloc(transactions, transaction_count * sizeof(struct Transaction *));
  }

  const uint32_t id = transaction_count - 1;
  transactions[id] = malloc(sizeof(struct Transaction));
  transactions[id]->client = client;
  transactions[id]->command = command;
  transactions[id]->password = client->password;

  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
}

// Run by thread, no need mutex
void remove_transaction(struct Transaction *transaction) {
  const uint64_t id = transaction - transactions[0];

  free_command_data(transaction->command);
  free(transaction);

  transaction_count -= 1;
  memcpy(transactions + id, transactions + id + 1, sizeof(struct Transaction *) * transaction_count);

  if (transaction_count == 0) {
    free(transactions);
  } else {
    transactions = realloc(transactions, sizeof(struct Transaction *) * transaction_count);
  }
}

void free_transactions() {
  if (transaction_count != 0) {
    for (uint32_t i = 0; i < transaction_count; ++i) {
      struct Transaction *transaction = transactions[i];

      free_command_data(transaction->command);
      free(transaction);
    }

    free(transactions);
  }
}
