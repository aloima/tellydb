#pragma once

#include "../utils/utils.h"

#include <stdint.h>

struct KVPair {
  string_t key;
  uint64_t hashed;
  void *value;
  enum TellyTypes type;

  struct {
    bool enabled;
    uint64_t at;
  } expire;
};

int set_kv(struct KVPair *kv, const string_t key, void *value, const enum TellyTypes type, const uint64_t *expire_at_p);
void free_value(const enum TellyTypes type, void *value);
void free_kv(struct KVPair *kv);
