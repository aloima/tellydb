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

struct KVPair *get_kv_from_cache(string_t key) {
  uint64_t index = hash(key.value, key.len);
  struct BTreeValue *value = find_value_from_btree(cache, index, &key, (bool (*)(void *, void *)) check_correct_kv);

  if (value) return value->data;
  else return NULL;
}

bool delete_kv_from_cache(const char *key, const size_t length) {
  const uint64_t index = hash((char *) key, length);
  return delete_value_from_btree(cache, index, (void (*)(void *)) free_kv);
}

void free_cache() {
  free_btree(cache, (void (*)(void *)) free_kv);
}
