#pragma once

#include <stdint.h>

#define VECTOR_GROW_LOAD_FACTOR 0.75
#define VECTOR_GROW_MULTIPLIER 2.00

typedef struct {
  struct {
    uint64_t capacity;
    uint64_t count;
  } size;

  void **elements;
} Vector;

Vector *create_vector(const uint64_t capacity);
void destroy_vector(Vector *vector, void (*destroy_element)(void *element));

bool insert_into_vector(Vector *vector, void *element);
bool delete_from_vector(Vector *vector, void *element);

void foreach_vector(Vector *vector, void (*procedure)(void *element));
bool any_in_vector(Vector *vector, bool (*procedure)(void *element));
