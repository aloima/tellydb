#include "../../headers/telly.h"

static struct BTree *cache = NULL;

void create_cache() {
  cache = create_btree(3);
}

struct BTree *get_cache() {
  return cache;
}

struct KVPair *get_kv_from_cache(const char *key) {
  return find_kv_from_btree(cache, key);
}

void free_cache() {
  free_btree(cache);
}
