#include "../../headers/telly.h"

// For EBUSY enum
#include <errno.h> // IWYU pragma: keep
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include <pthread.h>

struct Transaction *transactions;

uint64_t processed_transaction_count = 0;
uint32_t transaction_count = 0;
uint32_t at = 0;
uint32_t end = 0;
struct Configuration *conf;

pthread_t thread;
pthread_cond_t cond;
pthread_mutex_t mutex;

void *transaction_thread() {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  while (true) {
    pthread_mutex_lock(&mutex);
    while (transaction_count == 0) pthread_cond_wait(&cond, &mutex);

    struct Transaction *transaction = &transactions[at];
    execute_command(transaction);
    remove_transaction(transaction);
    at = ((at + 1) % conf->max_transactions);

    pthread_mutex_unlock(&mutex);
  }

  return NULL;
}

uint32_t get_transaction_count() {
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
  transactions = calloc(conf->max_transactions, sizeof(struct Transaction));

  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
  pthread_create(&thread, NULL, transaction_thread, NULL);
  pthread_detach(thread);
}

bool add_transaction(struct Client *client, struct Command *command, commanddata_t data) {
  if (transaction_count == conf->max_transactions) {
    return false;
  }

  pthread_mutex_lock(&mutex);

  struct Transaction *transaction = &transactions[end];
  transaction_count += 1;
  end = ((end + 1) % conf->max_transactions);

  transaction->client = client;
  transaction->command = command;
  transaction->data = data;
  transaction->password = client->password;
  transaction->database = client->database;

  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);

  return true;
}

// Run by thread, no need mutex
void remove_transaction(struct Transaction *transaction) {
  transaction_count -= 1;
  processed_transaction_count += 1;
  free_command_data(transaction->data);
  transaction->command = NULL;
}

void free_transactions() {
  for (uint32_t i = 0; i < conf->max_transactions; ++i) {
    struct Transaction transaction = transactions[i];

    if (transaction.command) {
      free_command_data(transaction.data);
    }
  }

  free(transactions);
}
