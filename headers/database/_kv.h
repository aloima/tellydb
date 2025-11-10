#pragma once

#include "../utils/utils.h"

#include <stdbool.h>
#include <stdint.h>

struct KVPair {
  string_t key;
  uint64_t hashed;
  void *value;
  enum TellyTypes type;
};

void set_kv(struct KVPair *kv, const string_t key, void *value, const enum TellyTypes type);
void free_value(const enum TellyTypes type, void *value);
void free_kv(struct KVPair *kv);
