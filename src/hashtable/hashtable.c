#include "../../headers/hashtable.h"
#include "../../headers/utils.h"

#include <stdlib.h>
#include <stdint.h>

struct HashTable *create_hashtable(const uint32_t default_size) {
  struct HashTable *table = malloc(sizeof(struct HashTable));
  table->fvs = calloc(default_size, sizeof(struct FVPair *));
  table->size.allocated = default_size;
  table->size.all = 0;
  table->size.filled = 0;

  return table;
}

void resize_hashtable(struct HashTable *table, const uint32_t size) {
  struct FVPair **fvs = calloc(size, sizeof(struct FVPair *));
  table->size.filled = 0;

  for (uint32_t i = 0; i < table->size.allocated; ++i) {
    struct FVPair *fv = table->fvs[i];

    while (fv) {
      struct FVPair *next = fv->next;
      fv->next = NULL;
      const uint32_t index = fv->hash % size;
      struct FVPair **area = &fvs[index];

      if (!*area) table->size.filled += 1;
      while (*area) area = &(*area)->next;

      *area = fv;
      fv = next;
    }
  }

  free(table->fvs);
  table->size.allocated = size;
  table->fvs = fvs;
}

struct FVPair *get_fv_from_hashtable(struct HashTable *table, char *name) {
  const uint32_t index = hash(name) % table->size.allocated;
  struct FVPair *fv = table->fvs[index];

  while (fv && !streq(fv->name.value, name)) fv = fv->next;
  return fv;
}

void free_hashtable(struct HashTable *table) {
  const uint32_t allocated_size = table->size.allocated;

  for (uint32_t i = 0; i < allocated_size; ++i) {
    struct FVPair *fv = table->fvs[i];

    while (fv) {
      struct FVPair *next = fv->next;
      free_fv(fv);
      fv = next;
    }
  }

  free(table->fvs);
  free(table);
}
