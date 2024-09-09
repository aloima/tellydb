#include "../../headers/telly.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

void set_string(string_t *data, char *value, int32_t len) {
  data->len = len == -1 ? strlen(value) : (uint32_t) len;
  const uint32_t size = data->len + 1;

  if (data->value != NULL) {
    data->value = realloc(data->value, size);
  } else {
    data->value = malloc(size);
  }

  memcpy(data->value, value, size);
}
