#include "../../headers/telly.h"

#include <stdlib.h>
#include <stdint.h>

uint64_t hash(char *key) {
  uint64_t hash = 5381;
  char c;

  while ((c = *key++)) hash = ((hash << 5) + hash) + c;

  return hash;
}

struct HashTable *create_hashtable(uint64_t default_size, double grow_factor) {
  struct HashTable *table = malloc(sizeof(struct HashTable));
  table->pairs = calloc(default_size, sizeof(struct FVPair *));
  table->count = 0;
  table->size = default_size;
  table->grow_factor = grow_factor;

  return table;
}

struct FVPair *get_fv_from_hashtable(struct HashTable *table, char *name) {
  const uint64_t index = hash(name) % table->size;
  return table->pairs[index];
}

void free_hashtable(struct HashTable *table) {
  for (uint64_t i = 0; i < table->size; ++i) {
    struct FVPair *pair = table->pairs[i];
    if (pair) free_fv(pair);
  }

  free(table->pairs);
  free(table);
}
