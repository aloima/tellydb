#include "../../headers/utils.h"

#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>
#include <math.h>

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

uint32_t get_digit_count(int32_t number) {
  if (number == 0) {
    return 1;
  } else if (number < 0) {
    return 2 + log10(number * -1);
  } else {
    return 1 + log10(number);
  }
}
