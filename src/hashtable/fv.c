#include "../../headers/telly.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void set_fv_value(struct FVPair *fv, void *value) {
  switch (fv->type) {
    case TELLY_STR:
      set_string(&fv->value.string, value, strlen(value), true);
      break;

    case TELLY_INT:
      fv->value.integer = *((int *) value);
      break;

    case TELLY_BOOL:
      fv->value.boolean = *((bool *) value);
      break;

    case TELLY_NULL:
      fv->value.null = NULL;
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
