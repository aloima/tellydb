#include "../../headers/telly.h"

#include <stdlib.h>

void free_htfield(struct HashTableField *field) {
  if (field->type == TELLY_STR) {
    string_t *string = field->value;
    free(string->value);
  }

  if (field->type != TELLY_NULL) free(field->value);
  free(field->name.value);
  free(field);
}
