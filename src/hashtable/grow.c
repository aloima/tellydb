#include "../../headers/hashtable.h"
#include "../../headers/utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

void set_fv_of_hashtable(struct HashTable *table, char *name, void *value, enum TellyTypes type) {
  // if (table->size.filled == table->size.allocated) grow_hashtable(table);

  const uint64_t index = hash(name) % table->size.allocated;
  table->size.all += 1;
  table->size.filled += 1;

  struct FVPair *fv;
  bool found = false;

  if ((fv = table->fvs[index])) {
    do {
      if (streq(fv->name.value, name)) {
        found = true;
        break;
      } else if (fv->next) fv = fv->next;
      else break;
    } while (fv);
  }

  if (fv) {
    if (found) {
      if (fv->type == TELLY_STR && type != TELLY_STR) free(fv->value.string.value);
      fv->type = type;
      set_fv_value(fv, value);
    } else {
      fv->next = malloc(sizeof(struct FVPair));
      fv->next->type = type;
      fv->next->next = NULL;

      const uint32_t len = strlen(name);
      const uint32_t size = len + 1;
      fv->next->name.len = len;
      fv->next->name.value = malloc(size);
      memcpy(fv->next->name.value, name, size);

      set_fv_value(fv->next, value);
    }
  } else {
    fv = malloc(sizeof(struct FVPair));
    fv->type = type;
    fv->next = NULL;
    
    const uint32_t len = strlen(name);
    const uint32_t size = len + 1;
    fv->name.len = len;
    fv->name.value = malloc(size);
    memcpy(fv->name.value, name, size);
    set_fv_value(fv, value);

    table->fvs[index] = fv;
  }
}
