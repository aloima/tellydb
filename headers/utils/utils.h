#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

#include <gmp.h>

#include "notifier.h"   // IWYU pragma: export
#include "tqueue.h"     // IWYU pragma: export
#include "number.h"     // IWYU pragma: export
#include "hashset.h"    // IWYU pragma: export
#include "hashtable.h"  // IWYU pragma: export
#include "vector.h"     // IWYU pragma: export
#include "linkedlist.h" // IWYU pragma: export
#include "string.h"     // IWYU pragma: export
#include "arena.h"      // IWYU pragma: export
#include "queue.h"      // IWYU pragma: export
#include "logging.h"    // IWYU pragma: export

#define ATOMIC_CAS_WEAK atomic_compare_exchange_weak_explicit

enum TellyTypes : uint8_t {
  TELLY_UNKNOWN,
  TELLY_NULL,
  TELLY_INT,
  TELLY_DOUBLE,
  TELLY_STR,
  TELLY_BOOL,
  TELLY_HASHTABLE,
  TELLY_LIST
};

static constexpr int64_t PRIMARY_TYPE_SIZE_TABLE[] = {
  [TELLY_NULL]      = 0,
  [TELLY_INT]       = sizeof(mpz_t),
  [TELLY_DOUBLE]    = sizeof(mpf_t),
  [TELLY_STR]       = sizeof(string_t),
  [TELLY_BOOL]      = sizeof(bool),
  [TELLY_HASHTABLE] = -1,
  [TELLY_LIST]      = -1
};

void memcpy_aligned(void *restrict dest, const void *restrict src, size_t n);
void memset_aligned(void *s, int c, size_t n);
int open_file(const char *file, int flags);
