#include "../../headers/hashtable.h"
#include "../../headers/utils.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

void add_fv_to_hashtable(struct HashTable *table, const string_t name, void *value, const enum TellyTypes type) {
  const uint64_t hashed = hash(name.value);
  uint32_t index = hashed % table->size.allocated;

  table->size.all += 1;

  struct FVPair *fv;
  bool found = false;

  if ((fv = table->fvs[index])) {
    do {
      if (streq(fv->name.value, name.value)) {
        found = true;
        break;
      } else if (fv->next) fv = fv->next;
      else break;
    } while (fv);
  } else {
    table->size.filled += 1;
  }

  if (table->size.allocated == table->size.filled) {
    resize_hashtable(table, (table->size.allocated * (1 + table->grow_factor)));
    index = hashed % table->size.allocated;

    if (!table->fvs[index]) table->size.filled += 1;
  }

  if (fv) {
    if (found) {
      if (fv->type == TELLY_STR && type != TELLY_STR) free(fv->value.string.value);
      fv->type = type;
      set_fv_value(fv, value);
    } else {
      fv->next = malloc(sizeof(struct FVPair));
      fv = fv->next;
    }
  } else {
    fv = malloc(sizeof(struct FVPair));
    table->fvs[index] = fv;
  }

  fv->type = type;
  fv->hash = hashed;
  fv->next = NULL;

  const uint32_t size = name.len + 1;
  fv->name.len = name.len;
  fv->name.value = malloc(size);
  memcpy(fv->name.value, name.value, size);

  set_fv_value(fv, value);
}

bool del_fv_to_hashtable(struct HashTable *table, const string_t name) {
  const uint64_t hashed = hash(name.value);
  uint32_t index = hashed % table->size.allocated;

  struct FVPair *fv;
  struct FVPair *prev = NULL;

  if ((fv = table->fvs[index])) {
    do {
      if (streq(fv->name.value, name.value)) {
        // b is element will be deleted
        if (prev) { // a b a or a a b
          prev->next = fv->next;
        } else if (!fv->next) { // b or ~~a a b~~
          table->size.filled -= 1;
          table->fvs[index] = NULL;

          const uint32_t size = (table->size.allocated * table->shrink_factor);

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
