#include <telly.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

void set_field_of_hashtable(struct HashTable *table, const string_t name, void *value, const enum TellyTypes type) {
  if (table->size.used == table->size.capacity) {
    resize_hashtable(table, (table->size.capacity * HASHTABLE_GROW_FACTOR));
  }

  const uint64_t hashed = hash(name.value, name.len);
  uint32_t index = hashed % table->size.capacity;

  struct HashTableField *field;


  do {
    field = table->fields[index];

    if (field) {
      if ((name.len == field->name.len) && (memcmp(field->name.value, name.value, name.len) == 0)) {
        if (field->type == TELLY_STR) {
          string_t *string = field->value;
          free(string->value);
        }

        if (field->type != TELLY_NULL) {
          free(field->value);
        }

        break;
      }

      index += 1;
    }
  } while (field && index != table->size.capacity);

  if (field == NULL) {
    table->size.used += 1;
    field = malloc(sizeof(struct HashTableField));

    field->name.len = name.len;
    field->name.value = malloc(name.len);
    memcpy(field->name.value, name.value, name.len);

    table->fields[index] = field;
  }

  field->type = type;
  field->value = value;
  field->hash = hashed;
}

bool del_field_to_hashtable(struct HashTable *table, const string_t name) {
  const uint64_t hashed = hash(name.value, name.len);
  uint32_t index = hashed % table->size.capacity;

  struct HashTableField *field;

  while (index != table->size.capacity && (field = table->fields[index])) {
    if ((name.len == field->name.len) && (memcmp(field->name.value, name.value, name.len) == 0)) {
      table->size.used -= 1;
      table->fields[index] = NULL;

      const uint32_t size = (table->size.capacity * HASHTABLE_SHRINK_FACTOR);

      if (size == table->size.used) {
        resize_hashtable(table, size);
      }

      free_htfield(field);
      return true;
    } else {
      index += 1;
    }
  }

  return false;
}
