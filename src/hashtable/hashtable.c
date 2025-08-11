#include <telly.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

struct HashTable *create_hashtable(const uint32_t default_size) {
  struct HashTable *table = malloc(sizeof(struct HashTable));
  table->fields = calloc(default_size, sizeof(struct HashTableField *));
  table->size.allocated = default_size;
  table->size.all = 0;
  table->size.filled = 0;

  return table;
}

void resize_hashtable(struct HashTable *table, const uint32_t size) {
  struct HashTableField **data = calloc(size, sizeof(struct HashTableField *));
  table->size.filled = 0;

  for (uint32_t i = 0; i < table->size.allocated; ++i) {
    struct HashTableField *field = table->fields[i];

    while (field) {
      const uint32_t index = field->hash % size;
      struct HashTableField *next = field->next;

      if (!data[index]) {
        table->size.filled += 1;
        field->next = NULL;
      } else {
        field->next = data[index];
      }

      data[index] = field;
      field = next;
    }
  }

  free(table->fields);
  table->size.allocated = size;
  table->fields = data;
}

struct HashTableField *get_field_from_hashtable(struct HashTable *table, const string_t name) {
  const uint32_t index = hash(name.value, name.len) % table->size.allocated;
  struct HashTableField *field = table->fields[index];

  while (field && ((name.len != field->name.len) || (memcmp(field->name.value, name.value, name.len) != 0))) {
    field = field->next;
  }

  return field;
}

void free_hashtable(struct HashTable *table) {
  const uint32_t allocated_size = table->size.allocated;

  for (uint32_t i = 0; i < allocated_size; ++i) {
    struct HashTableField *field = table->fields[i];

    while (field) {
      struct HashTableField *next = field->next;
      free_htfield(field);
      field = next;
    }
  }

  free(table->fields);
  free(table);
}
