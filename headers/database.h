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
    int32_t pos;
  };

  void create_cache();
  struct BTree *get_cache();
  struct KVPair *get_kv_from_cache(const char *key);
  void free_cache();

  struct KVPair *get_data(char *key, struct Configuration *conf);
  void set_data(struct KVPair pair, struct Configuration *conf);
  void save_data();

  void set_kv(struct KVPair *pair, char *key, void *value, enum TellyTypes type);
  void *get_kv_val(struct KVPair *pair, enum TellyTypes type);
  void free_kv(struct KVPair *pair);

  void open_database_fd(const char *filename);
  int get_database_fd();
  void close_database_fd();
  char read_char(int fd);

  pthread_t create_transaction_thread(struct Configuration *config);
  void deactive_transaction_thread();

  uint32_t get_transaction_count();
  void add_transaction(struct Client *client, respdata_t *data);
  void remove_transaction(struct Transaction *transaction);
  void free_transactions();
#endif
