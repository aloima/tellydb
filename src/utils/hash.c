#include "../../headers/utils.h"

#include <stdint.h>

uint64_t hash(char *key) {
  uint64_t hash = 5381;
  char c;

  while ((c = *key++)) hash = ((hash << 5) + hash) + c;
  return hash;
}
