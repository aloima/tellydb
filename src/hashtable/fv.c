#include <telly.h>

void free_htfield(struct HashTableField *field) {
  free_value(field->type, field->value);
  free(field->name.value);
  free(field);
}
