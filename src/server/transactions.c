#include "../../headers/telly.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>

#include <bits/types/sigset_t.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>

struct Transaction *start = NULL;
struct Transaction *end = NULL;

uint32_t transaction_count = 0;
struct Configuration *conf;

pthread_t thread;
pthread_cond_t cond;
pthread_mutex_t mutex;

void *transaction_thread() {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGKILL);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  while (true) {
    pthread_mutex_lock(&mutex);
    while (transaction_count == 0) pthread_cond_wait(&cond, &mutex);

    struct Transaction *transaction = end;
    execute_command(transaction);
    remove_transaction(transaction);

    pthread_mutex_unlock(&mutex);
  }

  return NULL;
}

uint32_t get_transaction_count() {
  return transaction_count;
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

  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
  pthread_create(&thread, NULL, transaction_thread, NULL);
  pthread_detach(thread);
}

void add_transaction(struct Client *client, struct Command *command, commanddata_t *data) {
  pthread_mutex_lock(&mutex);
  transaction_count += 1;

  struct Transaction *transaction = malloc(sizeof(struct Transaction));
  transaction->client = client;
  transaction->command = command;
  transaction->data = data;
  transaction->password = client->password;
  transaction->database = client->database;

  if (start) start->prev = transaction;
  transaction->prev = NULL;
  transaction->next = start;
  start = transaction;
  if (end == NULL) end = transaction;

  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
}

// Run by thread, no need mutex
void remove_transaction(struct Transaction *transaction) {
  if (end == transaction) {
    end = transaction->prev;
  }

  if (transaction->prev) {
    transaction->prev->next = transaction->next;
  } else { // (start == transaction)
    start = transaction->next;
  }

  transaction_count -= 1;
  free_command_data(transaction->data);
  free(transaction);
}

void free_transactions() {
  struct Transaction *transaction;

  while ((transaction = start)) {
    start = start->next;

    free_command_data(transaction->data);
    free(transaction);
  }
}
