#pragma once

#include <stdint.h>

#define HASHSET_GROW_LOAD_FACTOR 0.75
#define HASHSET_GROW_MULTIPLIER 2.00

typedef struct {
  struct {
    uint64_t capacity;
    uint64_t count;
  } size;

  void **elements;
} HashSet;

HashSet *create_hashset(const uint64_t capacity);
void destroy_hashset(HashSet *set, void (*destroy_element)(void *element));

int insert_into_hashset(HashSet *set, void *element);
bool delete_from_hashset(HashSet *set, void *element);
bool exist_in_hashset(HashSet *set, void *element);
