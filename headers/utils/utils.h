#pragma once

#include <stddef.h>
#include <stdint.h>

struct LinkedListNode {
  void *data;
  void *next;
};

enum TellyTypes : uint8_t {
  TELLY_NULL,
  TELLY_INT,
  TELLY_DOUBLE,
  TELLY_STR,
  TELLY_BOOL,
  TELLY_HASHTABLE,
  TELLY_LIST
};

void memcpy_aligned(void *restrict dest, const void *restrict src, size_t n);
int open_file(const char *file, int flags);

#include "integer.h" // IWYU pragma: export
#include "string.h"  // IWYU pragma: export
#include "logging.h" // IWYU pragma: export
