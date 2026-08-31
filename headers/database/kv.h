#pragma once

#include "../utils/utils.h"

#include <stdint.h>

typedef enum {
  EXPIRY_SYSCALL_ERROR = -2,
  EXPIRY_DELETING_ERROR = -1,
  EXPIRY_NOT_EXPIRED = 0,
  EXPIRY_EXPIRED = 1
} ExpiryState;

typedef struct {
  void *data;
  TellyType type;
} Value;

typedef struct {
  bool enabled;
  uint64_t at;
} Expiry;

typedef struct {
  string_t key;
  Value value;
  Expiry expiry;
} KeyValue;

typedef struct {
  string_t name;
  Value value;
} NameValue;

// Compatibility layer for HashTable and NameValue
typedef struct {
  string_t *key;
  NameValue *value;
} HashTableNameValue;

// Compatibility layer for HashTable and KeyValue
typedef struct {
  string_t *key;
  KeyValue *value;
} HashTableKeyValue;

int set_kv(KeyValue *kv, const string_t key, void *value, const TellyType type, const uint64_t *expire_at);
void free_value(Value value);
void free_namevalue(void *data);
void free_kv(KeyValue *kv);
void free_hashtablekeyvalue(HashTableElement element);

// Compatibility layer for LinkedList and Database Value
void free_list_value(void *data);
