#include <stdint.h>
#include <telly.h>

Vector *create_vector(const uint64_t capacity) {
  Vector *vector;
  if (amalloc(vector, Vector, 1) != 0)
    return NULL;

  if (amalloc(vector->elements, void *, capacity) != 0) {
    free(vector);
    return NULL;
  }

  for (uint64_t i = 0; i < capacity; ++i) {
    vector->elements[i] = NULL;
  }

  vector->size.capacity = capacity;
  vector->size.count = 0;

  return vector;
}

static int grow_vector(Vector *vector) {
  const uint64_t old_capacity = vector->size.capacity;

  if (vector->size.count < (old_capacity * VECTOR_GROW_LOAD_FACTOR))
    return 0;

  const uint64_t new_capacity = old_capacity * VECTOR_GROW_MULTIPLIER;
  void **elements;

  if (amalloc(elements, void *, new_capacity) != 0)
    return -1;

  memcpy(elements, vector->elements, old_capacity * sizeof(void *));

  free(vector->elements);
  vector->elements = elements;
  vector->size.capacity = new_capacity;

  return 1;
}

bool insert_into_vector(Vector *vector, void *element) {
  // Guaranteed that capacity is enough
  if (grow_vector(vector) < 0)
    return false;

  vector->elements[vector->size.count++] = element;
  return true;
}

bool delete_from_vector(Vector *vector, void *element) {
  const uint64_t count = vector->size.count;
  void **elements = vector->elements;

  for (uint64_t i = 0; i < count; ++i) {
    if (elements[i] != element) continue;

    memcpy(elements + i, elements + i + 1, sizeof(void *) * (count - i - 1));
    vector->size.count -= 1;

    return true;
  }

  return false;
}

void foreach_vector(Vector *vector, void (*procedure)(void *element)) {
  for (uint64_t i = 0; i < vector->size.count; ++i) {
    procedure(vector->elements[i]);
  }
}

bool any_in_vector(Vector *vector, bool (*procedure)(void *element)) {
  for (uint64_t i = 0; i < vector->size.count; ++i) {
    void *element = vector->elements[i];

    if (procedure(element))
      return true;
  }

  return false;
}

void clear_vector(Vector *vector, void (*destroy_element)(void *element)) {
  if (destroy_element != NULL)
    foreach_vector(vector, destroy_element);

  const uint64_t capacity = vector->size.capacity;
  vector->size.count = 0;

  for (uint64_t i = 0; i < capacity; ++i) {
    vector->elements[i] = NULL;
  }
}

void destroy_vector(Vector *vector, void (*destroy_element)(void *element)) {
  if (destroy_element != NULL)
    foreach_vector(vector, destroy_element);

  free(vector->elements);
  free(vector);
}
