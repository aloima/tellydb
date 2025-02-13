#include "../../headers/telly.h"

#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>

bool is_integer(const char *value) {
  char *_value = (char *) value;

  if (*value == '-') {
    _value += 1;

    if (*_value == '0' || *_value == '\0') return false;
  }

  while (isdigit(*_value)) {
    _value += 1;
  }

  return (_value != value) && (*_value == 0x00);
}

void number_pad(char *res, const uint32_t value) {
  if (value < 10) {
    res[0] = '0';
    res[1] = (value + 48);
  } else if (value < 100) {
    res[0] = ((value / 10) + 48);
    res[1] = ((value % 10) + 48);
  }

  res[2] = '\0';
}
