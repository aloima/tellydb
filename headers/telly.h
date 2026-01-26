#pragma once

#include <stdlib.h> // IWYU pragma: export

#define VERY_LIKELY(x) (__builtin_expect_with_probability(!!(x), 1, 0.999))
#define VERY_UNLIKELY(x) (__builtin_expect_with_probability(!!(x), 0, 0.999))
#define min(a, b) ((a) > (b) ? (b) : (a))
#define max(a, b) ((a) > (b) ? (a) : (b))

#if defined(__x86_64__) || defined(__i386__)
  #define cpu_relax() __asm__ __volatile__("pause\n" ::: "memory")
#elif defined(__aarch64__)
  #define cpu_relax() __asm__ __volatile__("yield\n" ::: "memory")
#endif

// Aligned memory allocation
#define amalloc(value, type, count) posix_memalign((void **) &(value), alignof(typeof(type)), (count) * sizeof(type))

#define FLOAT_PRECISION 1024

#include "auth.h"              // IWYU pragma: export
#include "commands/commands.h" // IWYU pragma: export
#include "config.h"            // IWYU pragma: export
#include "database/database.h" // IWYU pragma: export
#include "hashtable.h"         // IWYU pragma: export
#include "resp/resp.h"         // IWYU pragma: export
#include "server/server.h"     // IWYU pragma: export
#include "transactions.h"      // IWYU pragma: export
#include "utils/utils.h"       // IWYU pragma: export
