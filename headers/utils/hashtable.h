#pragma once

#include <stdint.h>

#define HASHTABLE_GROW_LOAD_FACTOR 0.75
#define HASHTABLE_GROW_MULTIPLIER 2.00

typedef struct HashTableElement {
  void *key;
  void *value;
} HashTableElement;

typedef struct {
  struct {
    uint64_t capacity;
    uint64_t count;
  } size;

  HashTableElement *elements;
  uint64_t (*hash)(void *key);
  bool (*key_compare)(void *key_a, void *key_b);
} HashTable;

HashTable *create_hashtable(const uint64_t capacity, uint64_t (*hash)(void *), bool (*key_compare)(void *key_a, void *key_b));
void clear_hashtable(HashTable *table, void (*destroy_element)(HashTableElement element));
void destroy_hashtable(HashTable *table, void (*destroy_element)(HashTableElement element));

void foreach_hashtable(HashTable *table, void (*procedure)(HashTableElement element, void *external), void *external);

HashTableElement *insert_into_hashtable(HashTable *table, void *key, void *value);
bool delete_from_hashtable(HashTable *table, void *key);
HashTableElement *get_from_hashtable(HashTable *table, void *key);
bool exist_in_hashtable(HashTable *table, void *key);
