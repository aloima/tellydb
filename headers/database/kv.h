#pragma once

#include "../utils/utils.h"

#include <stdint.h>

typedef struct Value {
  void *data;
  enum TellyTypes type;
} Value;

typedef struct Expiry {
  bool enabled;
  uint64_t at;
} Expiry;

typedef struct KeyValue {
  string_t key;
  Value *value;
  Expiry expiry;
} KeyValue;

// Compatibility layer for HashTable and KeyValue
typedef struct HashTableKeyValue {
  string_t *key;
  KeyValue *value;
} HashTableKeyValue;

int set_kv(KeyValue *kv, const string_t key, void *value, const enum TellyTypes type, const uint64_t *expire_at);
void free_value(Value *value);
void free_kv(KeyValue *kv);
void free_hashtablekeyvalue(HashTableElement element);
