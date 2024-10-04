#include "../../headers/telly.h"

#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

void set_string(string_t *data, char *value, int32_t len, bool unset) {
  data->len = len == -1 ? strlen(value) : (uint32_t) len;
  const uint32_t size = data->len + 1;

  if (unset) {
    data->value = malloc(size);
  } else {
    data->value = realloc(data->value, size);
  }

  memcpy(data->value, value, size);
}

void to_uppercase(char *in, char *out) {
  while (*in != '\0') *(out++) = toupper(*(in++));
  *out = '\0';
}
