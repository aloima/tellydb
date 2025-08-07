#include <telly.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

void add_field_to_hashtable(struct HashTable *table, const string_t name, void *value, const enum TellyTypes type) {
  const uint64_t hashed = hash(name.value, name.len);
  uint32_t index = hashed % table->size.allocated;

  struct HashTableField *field;
  bool found = false;

  if ((field = table->fields[index])) {
    do {
      if ((name.len == field->name.len) && (memcmp(field->name.value, name.value, name.len) == 0)) {
        found = true;
        break;
      } else if (field->next) field = field->next;
      else break;
    } while (field);
  } else {
    table->size.filled += 1;
  }

  if (table->size.allocated == table->size.filled) {
    resize_hashtable(table, (table->size.allocated * HASHTABLE_GROW_FACTOR));
    index = hashed % table->size.allocated;

    if (!table->fields[index]) table->size.filled += 1;
  }

  if (field) {
    if (found) {
      if (field->type == TELLY_STR) {
        string_t *string = field->value;
        free(string->value);
      }

      if (field->type != TELLY_NULL) free(field->value);

      field->type = type;
      field->value = value;
    } else {
      table->size.all += 1;

      field->next = malloc(sizeof(struct HashTableField));
      field = field->next;

      field->type = type;
      field->value = value;
      field->hash = hashed;
      field->next = NULL;

      field->name.len = name.len;
      field->name.value = malloc(name.len);
      memcpy(field->name.value, name.value, name.len);
    }
  } else {
    table->size.all += 1;

    field = malloc(sizeof(struct HashTableField));
    field->type = type;
    field->value = value;
    field->hash = hashed;
    field->next = NULL;

    field->name.len = name.len;
    field->name.value = malloc(name.len);
    memcpy(field->name.value, name.value, name.len);

    table->fields[index] = field;
  }
}

bool del_field_to_hashtable(struct HashTable *table, const string_t name) {
  const uint64_t hashed = hash(name.value, name.len);
  uint32_t index = hashed % table->size.allocated;

  struct HashTableField *field;
  struct HashTableField *prev = NULL;

  if ((field = table->fields[index])) {
    do {
      if ((name.len == field->name.len) && (memcmp(field->name.value, name.value, name.len) == 0)) {
        // b is element will be deleted
        if (prev) { // a b a or a a b
          prev->next = field->next;
        } else if (!field->next) { // b
          table->size.filled -= 1;
          table->fields[index] = NULL;

          const uint32_t size = (table->size.allocated * HASHTABLE_SHRINK_FACTOR);

          if (size == table->size.filled) {
            resize_hashtable(table, size);
          }
        } else { // b a a
          table->fields[index] = field->next;
        }

        table->size.all -= 1;
        free_htfield(field);
        return true;
      } else if (field->next) {
        prev = field;
        field = field->next;
      } else {
        return false;
      }
    } while (field);
  }

  return false;
}
