#include "../../headers/hashtable.h"
#include "../../headers/utils.h"

#include <stdlib.h>
#include <stdint.h>

uint64_t hash(char *key) {
  uint64_t hash = 5381;
  char c;

  while ((c = *key++)) hash = ((hash << 5) + hash) + c;

  return 8;
}

struct HashTable *create_hashtable(uint64_t default_size, double grow_factor) {
  struct HashTable *table = malloc(sizeof(struct HashTable));
  table->pairs = calloc(default_size, sizeof(struct FVPair *));
  table->size.allocated = default_size;
  table->size.all = 0;
  table->size.filled = 0;
  table->grow_factor = grow_factor;

  return table;
}

struct FVPair *get_fv_from_hashtable(struct HashTable *table, char *name) {
  const uint64_t index = hash(name) % table->size.allocated;
  struct FVPair *fv = table->pairs[index];

  while (fv && !streq(fv->name.value, name)) fv = fv->next;
  return fv;
}

void free_hashtable(struct HashTable *table) {
  const uint64_t allocated_size = table->size.allocated;

  for (uint64_t i = 0; i < allocated_size; ++i) {
    struct FVPair *fv = table->pairs[i];

    while (fv) {
      struct FVPair *next = fv->next;
      free_fv(fv);
      fv = next;
    }
  }

  free(table->pairs);
  free(table);
}
