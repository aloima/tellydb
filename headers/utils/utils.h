#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

#define ATOMIC_CAS_WEAK atomic_compare_exchange_weak_explicit

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
void memset_aligned(void *s, int c, size_t n);
int open_file(const char *file, int flags);

#include "notifier.h"   // IWYU pragma: export
#include "tqueue.h"     // IWYU pragma: export
#include "integer.h"    // IWYU pragma: export
#include "hashset.h"    // IWYU pragma: export
#include "linkedlist.h" // IWYU pragma: export
#include "string.h"     // IWYU pragma: export
#include "arena.h"      // IWYU pragma: export
#include "queue.h"      // IWYU pragma: export
#include "logging.h"    // IWYU pragma: export
