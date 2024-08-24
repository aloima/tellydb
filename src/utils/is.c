#include "../../headers/telly.h"

#include <stdbool.h>
#include <ctype.h>

bool is_integer(char *value) {
  while (true) {
    if (!isdigit(*value)) return false;
    value += 1;
    if (*value == 0x00) return true;
  }

  return true;
}
