#include "../../headers/hashtable.h"
#include "../../headers/utils.h"

#include <stdint.h>
#include <stdlib.h>

void free_fv(struct FVPair *fv) {
  if (fv->type == TELLY_STR) {
    string_t *string = fv->value;
    free(string->value);
  }

  if (fv->type != TELLY_NULL) free(fv->value);
  free(fv->name.value);
  free(fv);
}
