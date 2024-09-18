#include "config.h"
#include "resp.h"
#include "utils.h"

#include <stdint.h>

#include <pthread.h>

#ifndef DATABASE_H
  #define DATABASE_H

  /* DATABASE */
  struct KVPair {
    string_t key;
    value_t value;
    enum TellyTypes type;
    int32_t pos;
  };

  void create_cache();
  struct BTree *get_cache();
  struct KVPair *get_kv_from_cache(const char *key);
  void free_cache();

  struct KVPair *get_data(char *key, struct Configuration *conf);
  struct KVPair *set_data(struct KVPair pair, struct Configuration *conf);
  void save_data();

  void set_kv(struct KVPair *pair, char *key, void *value, enum TellyTypes type);
  void *get_kv_val(struct KVPair *pair, enum TellyTypes type);
  void free_kv(struct KVPair *pair);
  /* /DATABASE */


  /* DATABASE FILE */
  void open_database_fd(const char *filename);
  int get_database_fd();
  void close_database_fd();
  char read_char(int fd);
  /* /DATABASE FULE */


  /* TRANSACTIONS */
  struct Transaction {
    struct Client *client;
    respdata_t *command;
  };

  pthread_t create_transaction_thread(struct Configuration *config);
  void deactive_transaction_thread();

  uint32_t get_transaction_count();
  void add_transaction(struct Client *client, respdata_t *data);
  void remove_transaction(struct Transaction *transaction);
  void free_transactions();
  /* /TRANSACTIONS */
#endif
