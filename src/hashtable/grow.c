#include "../../headers/telly.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

void set_fv_of_hashtable(struct HashTable *table, char *name, void *value, enum TellyTypes type) {
  // if (table->size.filled == table->size.allocated) grow_hashtable(table);

  const uint64_t index = hash(name) % table->size.allocated;
  table->size.all += 1;
  table->size.filled += 1;

  struct FVPair **fv = NULL;

  if (table->pairs[index]) {
    fv = &table->pairs[index];

    while (fv) {
      if (streq((*fv)->name.value, name)) break;
      else fv = &(*fv)->next;
    }
  }

  if (fv) {
    if ((*fv)->type == TELLY_STR && type != TELLY_STR) free((*fv)->value.string.value);
    (*fv)->type = type;
    set_fv_value(*fv, value);
  } else {
    *fv = malloc(sizeof(struct FVPair));
    (*fv)->type = type;
    set_string(&(*fv)->name, name, strlen(name), true);
    set_fv_value(*fv, value);
    (*fv)->next = NULL;
  }
}
