#include <telly.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

static inline uint32_t add_to_index(const uint32_t index, const uint32_t capacity) {
  return ((index + 1) % capacity);
}

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

    index = add_to_index(index, capacity);
  }

  if (field == NULL) {
    table->size.used += 1;
    field = malloc(sizeof(struct HashTableField));
    if (!field) return;

    field->name.value = malloc(name.len);
    if (!field->name.value) {
      free(field);
      return;
    }
    field->name.len = name.len;

    memcpy(field->name.value, name.value, name.len);

    table->fields[index] = field;
  }

  field->type = type;
  field->value = value;
  field->hash = hashed;
}

bool del_field_from_hashtable(struct HashTable *table, const string_t name) {
  const uint32_t capacity = table->size.capacity;
  const uint64_t hashed = hash(name.value, name.len);
  const uint32_t start_idx = (hashed % capacity);
  uint32_t index = start_idx;

  struct HashTableField *field;

  while ((field = table->fields[index]) && (field->hash == index)) {
    if ((name.len != field->name.len) || (memcmp(field->name.value, name.value, name.len) != 0)) {
      index = add_to_index(index, capacity);

      if (index == start_idx) {
        break;
      }

      continue;
    }

    const uint32_t size = (capacity * HASHTABLE_SHRINK_FACTOR);
    table->size.used -= 1;
    table->fields[index] = NULL; // when shrinked, table capacity may be 1. so, loop may be not executed.

    if (size == table->size.used) {
      table->fields[index] = NULL;
      resize_hashtable(table, size);
    } else {
      for (uint32_t i = add_to_index(index, capacity); i != index; (i = add_to_index(index, capacity))) {
        struct HashTableField *move = table->fields[i];
        const uint32_t prev = ((i == 0) ? (capacity - 1) : (i - 1));

        if (!move || (move->hash != field->hash)) {
          table->fields[prev] = NULL;
          break;
        }

        table->fields[prev] = move;
      }
    }

    free_htfield(field);
    return true;
  }

  return false;
}
