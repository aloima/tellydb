#include "../../headers/hashtable.h"
#include "../../headers/utils.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

void set_fv_value(struct FVPair *fv, void *value) {
  switch (fv->type) {
    case TELLY_STR: {
      const uint32_t len = strlen(value);
      const uint32_t size = len + 1;
      fv->value.string.len = len;
      fv->value.string.value = malloc(size);
      memcpy(fv->value.string.value, value, size);
      break;
    }

    case TELLY_NUM:
      fv->value.number = *((long *) value);
      break;

    case TELLY_BOOL:
      fv->value.boolean = *((bool *) value);
      break;

    default:
      break;
  }
}

void free_fv(struct FVPair *fv) {
  if (fv->type == TELLY_STR) free(fv->value.string.value);

  free(fv->name.value);
  free(fv);
}
