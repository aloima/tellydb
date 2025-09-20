#include <telly.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

void set_field_of_hashtable(struct HashTable *table, const string_t name, void *value, const enum TellyTypes type) {
  const uint32_t capacity = table->size.capacity;

  if (table->size.used == capacity) {
    resize_hashtable(table, (capacity * HASHTABLE_GROW_FACTOR));
  }

  const uint64_t hashed = hash(name.value, name.len);
  uint32_t index = hashed % capacity;

  struct HashTableField *field;

  while ((field = table->fields[index])) {
    if ((name.len == field->name.len) && (memcmp(field->name.value, name.value, name.len) == 0)) {
      free_value(field->type, field->value);
      break;
    }

    index = ((index + 1) % capacity);
  }

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
  const uint32_t capacity = table->size.capacity;
  const uint64_t hashed = hash(name.value, name.len);
  uint32_t index = hashed % capacity;

  struct HashTableField *field;

  while ((field = table->fields[index])) {
    if ((name.len != field->name.len) || (memcmp(field->name.value, name.value, name.len) != 0)) {
      index = ((index + 1) % capacity);
      continue;
    }

    const uint32_t size = (capacity * HASHTABLE_SHRINK_FACTOR);
    table->size.used -= 1;

    if (size == table->size.used) {
      table->fields[index] = NULL;
      resize_hashtable(table, size);
    } else {
      for (uint32_t i = (index + 1); i < capacity; ++i) {
        struct HashTableField *move = table->fields[i];

        if (!move || (move->hash != field->hash)) {
          table->fields[i - 1] = NULL; // table->fields[index] will be processed for valid situation
          break;
        }

        table->fields[i - 1] = move;
      }
    }

    free_htfield(field);
    return true;
  }

  return false;
}
