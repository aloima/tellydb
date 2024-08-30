#include "config.h"
#include "resp.h"
#include "utils.h"

#include <stdint.h>

#include <pthread.h>

#ifndef DATABASE_H
  #define DATABASE_H

  enum TellyTypes {
    TELLY_NULL = 1,
    TELLY_INT,
    TELLY_STR,
    TELLY_BOOL
  };

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

    enum TellyTypes type;
  };

  void create_cache();
  void free_cache();

  struct KVPair *get_data(char *key, struct Configuration *conf);
  void set_data(struct KVPair pair);
  void save_data();

  void open_database_file(const char *filename);
  void close_database_file();

  pthread_t create_transaction_thread(struct Configuration *config);

  uint32_t get_transaction_count();
  void add_transaction(struct Client *client, respdata_t data);
  void remove_transaction(struct Transaction *transaction);
  void free_transactions();
#endif
