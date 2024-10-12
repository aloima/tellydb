#pragma once

#include <stddef.h>

typedef struct String {
  char *value;
  size_t len;
} string_t;
