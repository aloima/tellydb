#include "../../headers/telly.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

void grow_hashtable(struct HashTable *table) {
  table->size *= (1.0 + table->grow_factor);
  table->pairs = realloc(table->pairs, table->size * sizeof(struct FVPair *));
  memset(table->pairs + table->count, 0, (table->size - table->count) * sizeof(struct FVPair *));

  uint64_t check = 0;

  for (uint64_t i = 0; i < table->count; ++i) {
    struct FVPair *data = table->pairs[check];
    const uint64_t new_index = hash(data->name.value) % table->size;

    if (new_index != 0) {
      check += 1;
      continue;
    } else {
      table->pairs[0] = table->pairs[new_index];
      table->pairs[new_index] = data;
    }
  }
}

void set_fv_of_hashtable(struct HashTable *table, char *name, void *value, enum TellyTypes type) {
  if (table->count == table->size) grow_hashtable(table);

  const uint64_t index = hash(name) % table->size;
  table->count += 1;

  struct FVPair *pair;

  if (table->pairs[index]) {
    pair = table->pairs[index];
    if (pair->type == TELLY_STR && type != TELLY_STR) free(pair->value.string.value);
    pair->type = type;
    set_fv_value(pair, value);
  } else {
    pair = malloc(sizeof(struct FVPair));
    pair->type = type;
    set_string(&pair->name, name, strlen(name), true);
    set_fv_value(pair, value);

    table->pairs[index] = pair;
  }
}
