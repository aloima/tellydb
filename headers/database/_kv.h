#pragma once

#include "../utils.h"

struct KVPair {
  string_t key;
  void *value;
  enum TellyTypes type;
};

void set_kv(struct KVPair *kv, const string_t key, void *value, const enum TellyTypes type);
bool check_correct_kv(struct KVPair *kv, string_t *key);
void free_kv(struct KVPair *kv);
