#include "../../headers/telly.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

static uint64_t hash(char *key) {
  uint64_t hash = 5381;
  char c;

  while ((c = *key++)) hash = ((hash << 5) + hash) + c;

  return hash;
}

static void set_fv_value(struct FVPair *pair, void *value) {
  switch (pair->type) {
    case TELLY_STR:
      set_string(&pair->value.string, value, strlen(value), true);
      break;

    case TELLY_INT:
      pair->value.integer = *((int *) value);
      break;

    case TELLY_BOOL:
      pair->value.boolean = *((bool *) value);
      break;

    case TELLY_NULL:
      pair->value.null = NULL;
      break;

    default:
      break;
  }
}

struct HashTable *create_hashtable(uint64_t default_size, double grow_factor) {
  struct HashTable *table = malloc(sizeof(struct HashTable));
  table->pairs = calloc(default_size, sizeof(struct FVPair *));
  table->count = 0;
  table->size = default_size;
  table->grow_factor = grow_factor;

  return table;
}

static void grow_hashtable(struct HashTable *table) {
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

struct FVPair *get_fv_from_hashtable(struct HashTable *table, char *name) {
  const uint64_t index = hash(name) % table->size;
  return table->pairs[index];
}



static void free_fv(struct FVPair *pair) {
  if (pair->type == TELLY_STR) {
    free(pair->value.string.value);
  }

  free(pair->name.value);
  free(pair);
}

void free_hashtable(struct HashTable *table) {
  for (uint64_t i = 0; i < table->size; ++i) {
    struct FVPair *pair = table->pairs[i];
    if (pair) free_fv(pair);
  }

  free(table->pairs);
  free(table);
}
