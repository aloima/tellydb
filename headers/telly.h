#pragma once

#define VERY_LIKELY(x) (__builtin_expect_with_probability(!!(x), 1, 0.999))
#define VERY_UNLIKELY(x) (__builtin_expect_with_probability(!!(x), 0, 0.999))
#define min(a, b) ((a) > (b) ? (b) : (a))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define FLOAT_PRECISION 1024

#include "io.h"                   // IWYU pragma: export
#include "auth.h"                 // IWYU pragma: export
#include "commands/commands.h"    // IWYU pragma: export
#include "config.h"               // IWYU pragma: export
#include "database/database.h"    // IWYU pragma: export
#include "hashtable.h"            // IWYU pragma: export
#include "resp.h"                 // IWYU pragma: export
#include "server/server.h"        // IWYU pragma: export
#include "transactions/public.h"  // IWYU pragma: export
#include "utils/utils.h"          // IWYU pragma: export
