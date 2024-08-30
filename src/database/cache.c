#include "../../headers/telly.h"

static struct BTree *cache = NULL;

void create_cache() {
  cache = create_btree(3);
}

struct BTree *get_cache() {
  return cache;
}

void free_cache() {
  free_btree(cache);
}
