// Includes hash table structure and its methods

#pragma once

#include "utils.h"

#include <stdint.h>

#define HASHTABLE_GROW_FACTOR 1.5
#define HASHTABLE_SHRINK_FACTOR 0.75

struct FVPair {
  string_t name;
  void *value;
  enum TellyTypes type;
  struct FVPair *next;
  uint64_t hash;
};

struct HashTableSize {
  uint32_t allocated; // total allocated size
  uint32_t filled; // filled allocated block count
  uint32_t all; // contains next values
};

struct HashTable {
  struct FVPair **fvs;
  struct HashTableSize size;
};

struct HashTable *create_hashtable(uint32_t default_size);
void resize_hashtable(struct HashTable *table, const uint32_t size);
struct FVPair *get_fv_from_hashtable(struct HashTable *table, const string_t name);
void free_hashtable(struct HashTable *table);

void free_fv(struct FVPair *fv);

void add_fv_to_hashtable(struct HashTable *table, const string_t name, void *value, const enum TellyTypes type);
bool del_fv_to_hashtable(struct HashTable *table, const string_t name);
