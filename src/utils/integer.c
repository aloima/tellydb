#include "../../headers/telly.h"

#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>
#include <math.h>

bool is_integer(char *value) {
  if (*value == '-') value += 1;

  while (true) {
    if (!isdigit(*value)) return false;
    value += 1;
    if (*value == 0x00) return true;
  }

  return true;
}

uint32_t get_digit_count(int32_t number) {
  if (number == 0) {
    return 1;
  } else if (number < 0) {
    return 1 + log10(number * -1);
  } else {
    return 1 + log10(number);
  }
}
