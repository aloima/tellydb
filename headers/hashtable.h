#pragma once

#include "utils.h"

#include <stdint.h>

#define HASHTABLE_GROW_FACTOR 1.5
#define HASHTABLE_SHRINK_FACTOR 0.75

struct HashTableField {
  string_t name;
  void *value;
  enum TellyTypes type;
  uint64_t hash;
};

struct HashTableSize {
  uint32_t capacity; // total allocated size
  uint32_t used; // filled allocated block count
};

struct HashTable {
  struct HashTableField **fields;
  struct HashTableSize size;
};

struct HashTable *create_hashtable(uint32_t default_size);
void resize_hashtable(struct HashTable *table, const uint32_t size);
struct HashTableField *get_field_from_hashtable(struct HashTable *table, const string_t name);
void free_hashtable(struct HashTable *table);

void free_htfield(struct HashTableField *field);

void set_field_of_hashtable(struct HashTable *table, const string_t name, void *value, const enum TellyTypes type);
bool del_field_to_hashtable(struct HashTable *table, const string_t name);
