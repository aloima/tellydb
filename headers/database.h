#include "resp.h"
#include "utils.h"

#include <stdint.h>

#include <pthread.h>

#ifndef DATABASE_H
  #define DATABASE_H

  #define TELLY_NULL 1
  #define TELLY_INT 2
  #define TELLY_STR 3
  #define TELLY_BOOL 4

  struct Transaction {
    struct Client *client;
    respdata_t *command;
  };

  struct KVPair {
    string_t key;

    union {
      string_t string;
      int integer;
      bool boolean;
      void *null;
    } value;

    uint32_t type;
  };

  pthread_t create_transaction_thread(struct Configuration *conf);

  uint32_t get_transaction_count();
  void add_transaction(struct Client *client, respdata_t data);
  void remove_transaction(struct Transaction *transaction);
  void free_transactions();
#endif
