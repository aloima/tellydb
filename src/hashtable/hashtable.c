#include <telly.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

struct HashTable *create_hashtable(const uint32_t size) {
  struct HashTable *table = malloc(sizeof(struct HashTable));
  table->fields = calloc(size, sizeof(struct HashTableField *));
  table->size.capacity = size;
  table->size.used = 0;

  return table;
}

void resize_hashtable(struct HashTable *table, const uint32_t size) {
  struct HashTableField **data = calloc(size, sizeof(struct HashTableField *));

  for (uint32_t i = 0; i < table->size.capacity; ++i) {
    struct HashTableField *field = table->fields[i];

    if (!field) {
      continue;
    }

    uint32_t index = field->hash % size;

    while (data[index]) {
      index = ((index + 1) % size);
    }

    data[index] = field;
  }

  free(table->fields);
  table->size.capacity = size;
  table->fields = data;
}

struct HashTableField *get_field_from_hashtable(struct HashTable *table, const string_t name) {
  const uint32_t capacity = table->size.capacity;
  uint32_t index = hash(name.value, name.len) % capacity;
  struct HashTableField *field;

  while ((field = table->fields[index])) {
    index = ((index + 1) % capacity);

    if ((name.len == field->name.len) && (memcmp(field->name.value, name.value, name.len) == 0)) {
      return field;
    }
  }

  return NULL;
}

void free_hashtable(struct HashTable *table) {
  const uint32_t capacity = table->size.capacity;

  for (uint32_t i = 0; i < capacity; ++i) {
    struct HashTableField *field = table->fields[i];

    if (field) {
      free_htfield(field);
    }
  }

  free(table->fields);
  free(table);
}
