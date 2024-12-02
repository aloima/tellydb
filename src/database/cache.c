#include "../../headers/database.h"
#include "../../headers/btree.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

static struct BTree *cache = NULL;

struct BTree *create_cache() {
  return (cache = create_btree(3));
}

struct BTree *get_cache() {
  return cache;
}

struct KVPair *get_kv_from_cache(const char *key, const size_t length) {
  uint64_t index = hash((char *) key, length);

  while (true) {
    struct BTreeValue *value = find_value_from_btree(cache, index);

    if (value) {
      struct KVPair *kv = value->data;

      if (strncmp(kv->key.value, key, length) == 0) return kv;
      else index += 1;
    } else return NULL;
  }
}

bool delete_kv_from_cache(const char *key, const size_t length) {
  const uint64_t index = hash((char *) key, length);
  return delete_value_from_btree(cache, index, (void (*)(void *)) free_kv);
}

void free_cache() {
  free_btree(cache, (void (*)(void *)) free_kv);
}
