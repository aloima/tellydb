#include "../../headers/database.h"
#include "../../headers/btree.h"

#include <stdint.h>
#include <stdbool.h>

static struct BTree *cache = NULL;

struct BTree *create_cache() {
  return (cache = create_btree(3));
}

struct BTree *get_cache() {
  return cache;
}

struct KVPair *get_kv_from_cache(const char *key) {
  uint64_t index = hash((char *) key);

  while (true) {
    struct BTreeValue *value = find_value_from_btree(cache, index);

    if (value) {
      struct KVPair *kv = value->data;

      if (streq(kv->key.value, key)) return kv;
      else index += 1;
    } else return NULL;
  }
}

bool delete_kv_from_cache(const char *key) {
  const uint64_t index = hash((char *) key);
  return delete_value_from_btree(cache, index);
}

void free_cache() {
  free_btree(cache, (void (*)(void *)) free_kv);
}

struct BTreeValue **get_sorted_kvs_by_pos_as_values(uint32_t *size) {
  struct BTreeValue **values = get_values_from_btree(cache, size);
  if (*size <= 1) return values;

  const uint32_t bound_parent = *size - 1;

  for (uint32_t i = 0; i < bound_parent; ++i) {
    const uint32_t bound = *size - 1 - i;

    for (uint32_t j = 0; j < bound; ++j) {
      struct BTreeValue *a = values[j];
      struct BTreeValue *b = values[j + 1];

      if (((struct KVPair *) a->data)->pos.start_at <= ((struct KVPair *) b->data)->pos.start_at) continue;
      values[j + 1] = a;
      values[j] = b;
    }
  }

  return values;
}
