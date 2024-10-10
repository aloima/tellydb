#include "../../headers/utils.h"

#include <ctype.h>

void to_uppercase(char *in, char *out) {
  while (*in != '\0') *(out++) = toupper(*(in++));
  *out = '\0';
}
