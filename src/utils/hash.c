#include <telly.h>

#include <stdint.h>

uint64_t hash(char *key, uint32_t length) {
  uint64_t hash = 5381;

  while (length--) {
    hash = ((hash << 5) + hash) + (*key++);
  }

  return hash;
}
