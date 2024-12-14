#include "../../headers/hashtable.h"
#include "../../headers/utils.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

void add_fv_to_hashtable(struct HashTable *table, const string_t name, void *value, const enum TellyTypes type) {
  const uint64_t hashed = hash(name.value, name.len);
  uint32_t index = hashed % table->size.allocated;

  struct FVPair *fv;
  bool found = false;

  if ((fv = table->fvs[index])) {
    do {
      if ((name.len == fv->name.len) && (memcmp(fv->name.value, name.value, name.len) == 0)) {
        found = true;
        break;
      } else if (fv->next) fv = fv->next;
      else break;
    } while (fv);
  } else {
    table->size.filled += 1;
  }

  if (table->size.allocated == table->size.filled) {
    resize_hashtable(table, (table->size.allocated * HASHTABLE_GROW_FACTOR));
    index = hashed % table->size.allocated;

    if (!table->fvs[index]) table->size.filled += 1;
  }

  if (fv) {
    if (found) {
      if (fv->type == TELLY_STR) {
        string_t *string = fv->value;
        free(string->value);
      }

      if (fv->type != TELLY_NULL) free(fv->value);

      fv->type = type;
      fv->value = value;
    } else {
      table->size.all += 1;

      fv->next = malloc(sizeof(struct FVPair));
      fv = fv->next;

      fv->type = type;
      fv->value = value;
      fv->hash = hashed;
      fv->next = NULL;

      fv->name.len = name.len;
      fv->name.value = malloc(name.len);
      memcpy(fv->name.value, name.value, name.len);
    }
  } else {
    table->size.all += 1;

    fv = malloc(sizeof(struct FVPair));
    fv->type = type;
    fv->value = value;
    fv->hash = hashed;
    fv->next = NULL;

    fv->name.len = name.len;
    fv->name.value = malloc(name.len);
    memcpy(fv->name.value, name.value, name.len);

    table->fvs[index] = fv;
  }
}

bool del_fv_to_hashtable(struct HashTable *table, const string_t name) {
  const uint64_t hashed = hash(name.value, name.len);
  uint32_t index = hashed % table->size.allocated;

  struct FVPair *fv;
  struct FVPair *prev = NULL;

  if ((fv = table->fvs[index])) {
    do {
      if ((name.len == fv->name.len) && (memcmp(fv->name.value, name.value, name.len) == 0)) {
        // b is element will be deleted
        if (prev) { // a b a or a a b
          prev->next = fv->next;
        } else if (!fv->next) { // b
          table->size.filled -= 1;
          table->fvs[index] = NULL;

          const uint32_t size = (table->size.allocated * HASHTABLE_SHRINK_FACTOR);

          if (size == table->size.filled) {
            resize_hashtable(table, size);
          }
        } else { // b a a
          table->fvs[index] = fv->next;
        }

        table->size.all -= 1;
        free_fv(fv);
        return true;
      } else if (fv->next) {
        prev = fv;
        fv = fv->next;
      } else {
        return false;
      }
    } while (fv);
  }

  return false;
}
