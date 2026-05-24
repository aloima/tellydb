#include <telly.h>

HashTable *create_hashtable(const uint64_t capacity, uint64_t (*hash)(void *), bool (*key_compare)(void *, void *)) {
  HashTable *table;
  if (amalloc(table, HashTable, 1) != 0)
    return NULL;

  if (amalloc(table->elements, HashTableElement, capacity) != 0) {
    free(table);
    return NULL;
  }

  for (uint64_t i = 0; i < capacity; ++i) {
    table->elements[i].key = NULL;
    table->elements[i].value = NULL;
  }

  table->hash = hash;
  table->key_compare = key_compare;
  table->size.capacity = capacity;
  table->size.count = 0;

  return table;
}

static int grow_hashtable(HashTable *table) {
  const uint64_t old_capacity = table->size.capacity;

  if (table->size.count < (old_capacity * HASHTABLE_GROW_LOAD_FACTOR))
    return 0;

  const uint64_t new_capacity = old_capacity * HASHTABLE_GROW_MULTIPLIER;
  HashTableElement *elements;

  if (amalloc(elements, HashTableElement, new_capacity) != 0)
    return -1;

  for (uint64_t i = 0; i < old_capacity; ++i) {
    HashTableElement element = table->elements[i];
    if (element.key == NULL) continue;

    uint64_t index = table->hash(element.key) % new_capacity;

    while (elements[index].key != NULL)
      index = (index + 1) % new_capacity;

    elements[index] = element;
  }

  free(table->elements);
  table->elements = elements;
  table->size.capacity = new_capacity;

  return 1;
}

HashTableElement *insert_into_hashtable(HashTable *table, void *key, void *value) {
  // Guaranteed that capacity is enough
  if (grow_hashtable(table) < 0)
    return NULL;

  const uint64_t capacity = table->size.capacity;
  const uint64_t start = table->hash(key) % capacity;
  uint64_t index = start;

  while (table->elements[index].key != NULL) {
    if (table->key_compare(table->elements[index].key, key))
      return &table->elements[index];

    index = (index + 1) % capacity;
  }

  table->elements[index].key = key;
  table->elements[index].value = value;
  table->size.count += 1;

  return &table->elements[index];
}

bool delete_from_hashtable(HashTable *table, void *key) {
  const uint64_t capacity = table->size.capacity;
  const uint64_t start = table->hash(key) % capacity;
  uint64_t deletion = start;

  while (!table->key_compare(table->elements[deletion].key, key)) {
    if (table->elements[deletion].key == NULL)
      return false;

    deletion = (deletion + 1) % capacity;

    // Element cannot be found even all hashtable was searched.
    if (deletion == start)
      return false;
  }

  table->size.count -= 1;
  uint64_t current = deletion;

  while (true) {
    table->elements[deletion].key = NULL;
    table->elements[deletion].value = NULL;

    // Checks whether deletion is succeed, otherwise finds ideal (current) element to move into elements[deletion]
    while (true) {
      current = (current + 1) % capacity;
      void *current_key = table->elements[current].key;

      if (current_key == NULL)
        return true;

      const uint64_t ideal = table->hash(current_key) % capacity;

      // If ideal is not in (deletion, current], move elements[current] into elements[deletion]
      if (deletion <= current) {
        if (!(deletion < ideal && ideal <= current))
          break;
      } else {
        if (!(deletion < ideal || ideal <= current))
          break;
      }
    }

    table->elements[deletion].key = table->elements[current].key;
    table->elements[deletion].value = table->elements[current].value;
    deletion = current;
  }

  return true;
}

bool exist_in_hashtable(HashTable *table, void *key) {
  const uint64_t capacity = table->size.capacity;
  const uint64_t start = table->hash(key) % capacity;
  uint64_t index = start;

  while (!table->key_compare(table->elements[index].key, key)) {
    if (table->elements[index].key == NULL)
      return false;

    index = (index + 1) % capacity;

    if (index == start)
      return false;
  }

  return true;
}

HashTableElement *get_from_hashtable(HashTable *table, void *key) {
  const uint64_t capacity = table->size.capacity;
  const uint64_t start = table->hash(key) % capacity;
  uint64_t index = start;

  while (table->key_compare(table->elements[index].key, key)) {
    if (table->elements[index].key == NULL)
      return NULL;

    index = (index + 1) % capacity;

    if (index == start)
      return NULL;
  }

  return &table->elements[index];
}

void foreach_hashtable(HashTable *table, void (*procedure)(HashTableElement element, void *external), void *external) {
  for (uint64_t i = 0; i < table->size.count; ++i) {
    if (table->elements[i].key == NULL) continue;
    procedure(table->elements[i], external);
  }
}

void clear_hashtable(HashTable *table, void (*destroy_element)(HashTableElement element)) {
  if (destroy_element != NULL)
    foreach_hashtable(table, (void (*)(HashTableElement element, void *)) destroy_element, NULL);

  const uint64_t capacity = table->size.capacity;
  table->size.count = 0;

  for (uint64_t i = 0; i < capacity; ++i) {
    table->elements->key = NULL;
    table->elements->value = NULL;
  }
}

void destroy_hashtable(HashTable *table, void (*destroy_element)(HashTableElement element)) {
  if (destroy_element != NULL) {
    for (uint64_t i = 0; i < table->size.count; ++i) {
      destroy_element(table->elements[i]);
    }
  }

  free(table->elements);
  free(table);
}
