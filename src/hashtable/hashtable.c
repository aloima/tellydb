#include <telly.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

struct HashTable *create_hashtable(const uint32_t default_size) {
  struct HashTable *table = malloc(sizeof(struct HashTable));
  table->fields = calloc(default_size, sizeof(struct HashTableField *));
  table->size.capacity = default_size;
  table->size.used = 0;

  return table;
}

void resize_hashtable(struct HashTable *table, const uint32_t size) {
  struct HashTableField **data = calloc(size, sizeof(struct HashTableField *));

  for (uint32_t i = 0; i < table->size.capacity; ++i) {
    struct HashTableField *field = table->fields[i];
    uint32_t index = field->hash % size;

    while (data[index]) {
      index += 1;
    }

    data[index] = table->fields[i];
  }

  free(table->fields);
  table->size.capacity = size;
  table->fields = data;
}

struct HashTableField *get_field_from_hashtable(struct HashTable *table, const string_t name) {
  uint32_t index = hash(name.value, name.len) % table->size.capacity;
  struct HashTableField *field;

  do {
    field = table->fields[index];
    index += 1;

    if (field && (name.len == field->name.len) && (memcmp(field->name.value, name.value, name.len) == 0)) {
      return field;
    }
  } while (index != table->size.capacity);

  return field;
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
