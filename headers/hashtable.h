#include "utils.h"
#include "database.h"

#include <stdint.h>

#ifndef HASHTABLE_H_
  #define HASHTABLE_H_

  struct FVPair {
    string_t name;
    union {
      string_t string;
      int integer;
      bool boolean;
      void *null;
    } value;
    enum TellyTypes type;
  };

  struct HashTable {
    struct FVPair **pairs;
    uint64_t count;
    uint64_t size;
    double grow_factor;
  };

  uint64_t hash(char *key);
  struct HashTable *create_hashtable(uint64_t default_size, double grow_factor);
  struct FVPair *get_fv_from_hashtable(struct HashTable *table, char *name);
  void free_hashtable(struct HashTable *table);

  void set_fv_value(struct FVPair *pair, void *value);
  void free_fv(struct FVPair *pair);

  void grow_hashtable(struct HashTable *table);
  void set_fv_of_hashtable(struct HashTable *table, char *name, void *value, enum TellyTypes type);
#endif
