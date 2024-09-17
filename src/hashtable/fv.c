#include "../../headers/telly.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void set_fv_value(struct FVPair *pair, void *value) {
  switch (pair->type) {
    case TELLY_STR:
      set_string(&pair->value.string, value, strlen(value), true);
      break;

    case TELLY_INT:
      pair->value.integer = *((int *) value);
      break;

    case TELLY_BOOL:
      pair->value.boolean = *((bool *) value);
      break;

    case TELLY_NULL:
      pair->value.null = NULL;
      break;

    default:
      break;
  }
}

void free_fv(struct FVPair *pair) {
  if (pair->type == TELLY_STR) {
    free(pair->value.string.value);
  }

  free(pair->name.value);
  free(pair);
}
