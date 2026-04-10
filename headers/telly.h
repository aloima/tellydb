#pragma once

// Standard headers
#include <math.h>      // IWYU pragma: export
#include <time.h>      // IWYU pragma: export
#include <stdio.h>     // IWYU pragma: export
#include <errno.h>     // IWYU pragma: export
#include <string.h>    // IWYU pragma: export
#include <limits.h>    // IWYU pragma: export
#include <stdlib.h>    // IWYU pragma: export
#include <assert.h>    // IWYU pragma: export
#include <signal.h>    // IWYU pragma: export
#include <stdint.h>    // IWYU pragma: export
#include <inttypes.h>  // IWYU pragma: export
#include <stdatomic.h> // IWYU pragma: export

// POSIX-specific headers
#include <fcntl.h>       // IWYU pragma: export
#include <alloca.h>      // IWYU pragma: export
#include <unistd.h>      // IWYU pragma: export
#include <pthread.h>     // IWYU pragma: export
#include <strings.h>     // IWYU pragma: export
#include <sys/mman.h>    // IWYU pragma: export
#include <sys/stat.h>    // IWYU pragma: export
#include <sys/types.h>   // IWYU pragma: export
#include <semaphore.h>   // IWYU pragma: export
#include <sys/socket.h>  // IWYU pragma: export
#include <netinet/in.h>  // IWYU pragma: export
#include <netinet/tcp.h> // IWYU pragma: export

// External library headers
#include <gmp.h>                // IWYU pragma: export
#include <openssl/ssl.h>        // IWYU pragma: export
#include <openssl/kdf.h>        // IWYU pragma: export
#include <openssl/lhash.h>      // IWYU pragma: export
#include <openssl/crypto.h>     // IWYU pragma: export
#include <openssl/params.h>     // IWYU pragma: export
#include <openssl/provider.h>   // IWYU pragma: export
#include <openssl/core_names.h> // IWYU pragma: export

// To guarantee code execution
#define GASSERT(actual, op, expected) do { \
  typeof(actual) __actual_val = (actual);   \
  typeof(expected) __expected_val = (expected);   \
  assert(__actual_val op __expected_val);        \
} while (0)

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

// telly headers
#include "auth.h"              // IWYU pragma: export
#include "commands/commands.h" // IWYU pragma: export
#include "config.h"            // IWYU pragma: export
#include "database/database.h" // IWYU pragma: export
#include "hashtable.h"         // IWYU pragma: export
#include "resp/resp.h"         // IWYU pragma: export
#include "server/server.h"     // IWYU pragma: export
#include "transactions.h"      // IWYU pragma: export
#include "utils/utils.h"       // IWYU pragma: export
