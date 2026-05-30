#pragma once

#include "../utils/utils.h"

#include <stdint.h>

typedef enum ExpiryState {
  EXPIRY_SYSCALL_ERROR = -2,
  EXPIRY_DELETING_ERROR = -1,
  EXPIRY_NOT_EXPIRED = 0,
  EXPIRY_EXPIRED = 1
} ExpiryState;

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
  Value value;
  Expiry expiry;
} KeyValue;

typedef struct NameValue {
  string_t name;
  Value value;
} NameValue;

// Compatibility layer for HashTable and NameValue
typedef struct HashTableNameValue {
  string_t *key;
  NameValue *value;
} HashTableNameValue;

// Compatibility layer for HashTable and KeyValue
typedef struct HashTableKeyValue {
  string_t *key;
  KeyValue *value;
} HashTableKeyValue;

int set_kv(KeyValue *kv, const string_t key, void *value, const enum TellyTypes type, const uint64_t *expire_at);
void free_value(Value value);
void free_kv(KeyValue *kv);
void free_hashtablekeyvalue(HashTableElement element);

// Compatibility layer for LinkedList and Database Value
void free_list_value(void *data);
