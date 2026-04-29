#include <telly.h>

HashSet *create_hashset(const uint64_t capacity) {
  HashSet *set;
  if (amalloc(set, HashSet, 1) != 0)
    return NULL;

  if (amalloc(set->elements, void *, capacity) != 0) {
    free(set);
    return NULL;
  }

  for (uint64_t i = 0; i < capacity; ++i) {
    set->elements[i] = NULL;
  }

  set->size.capacity = capacity;
  set->size.count = 0;

  return set;
}

static int grow_hashset(HashSet *set) {
  const uint64_t old_capacity = set->size.capacity;

  if (set->size.count < (old_capacity * HASHSET_GROW_LOAD_FACTOR))
    return 0;

  const uint64_t new_capacity = old_capacity * HASHSET_GROW_MULTIPLIER;
  void **elements;

  if (amalloc(elements, void *, new_capacity) != 0)
    return -1;

  for (uint64_t i = 0; i < old_capacity; ++i) {
    void *element = set->elements[i];
    if (element == NULL) continue;

    uint64_t index = ((uintptr_t) element) % new_capacity;

    while (elements[index] != NULL)
      index = (index + 1) % new_capacity;

    elements[index] = element;
  }

  free(set->elements);
  set->elements = elements;
  set->size.capacity = new_capacity;

  return 1;
}

int insert_into_hashset(HashSet *set, void *element) {
  // Guaranteed that capacity is enough
  if (grow_hashset(set) < 0)
    return -2;

  const uint64_t capacity = set->size.capacity;
  const uint64_t start = ((uintptr_t) element) % capacity;
  uint64_t index = start;

  while (set->elements[index] != NULL) {
    if (set->elements[index] == element)
      return -1;

    index = (index + 1) % capacity;
  }

  set->elements[index] = element;
  set->size.count += 1;

  return 0;
}

bool delete_from_hashset(HashSet *set, void *element) {
  const uint64_t capacity = set->size.capacity;
  const uint64_t start = ((uintptr_t) element) % capacity;
  uint64_t deletion = start;

  while (set->elements[deletion] != element) {
    if (set->elements[deletion] == NULL)
      return false;

    deletion = (deletion + 1) % capacity;

    // Element cannot be found even all hashset was searched.
    if (deletion == start)
      return false;
  }

  set->size.count -= 1;
  uint64_t current = deletion;

  while (true) {
    set->elements[deletion] = NULL;

    // Checks whether deletion is succeed, otherwise finds ideal (current) element to move into elements[deletion]
    while (true) {
      current = (current + 1) % capacity;
      void *current_element = set->elements[current];

      if (current_element == NULL)
        return true;

      const uint64_t ideal = ((uintptr_t) current_element) % capacity;

      // If ideal is not in (deletion, current], move elements[current] into elements[deletion]
      if (deletion <= current) {
        if (!(deletion < ideal && ideal <= current))
          break;
      } else {
        if (!(deletion < ideal || ideal <= current))
          break;
      }
    }

    set->elements[deletion] = set->elements[current];
    deletion = current;
  }

  return true;
}

bool exist_in_hashset(HashSet *set, void *element) {
  const uint64_t capacity = set->size.capacity;
  const uint64_t start = ((uintptr_t) element) % capacity;
  uint64_t index = start;

  while (set->elements[index] != element) {
    if (set->elements[index] == NULL)
      return false;

    index = (index + 1) % capacity;

    if (index == start)
      return false;
  }

  return true;
}

void destroy_hashset(HashSet *set, void (*destroy_element)(void *element)) {
  if (destroy_element != NULL) {
    for (uint64_t i = 0; i < set->size.count; ++i) {
      destroy_element(set->elements[i]);
    }
  }

  free(set->elements);
  free(set);
}
