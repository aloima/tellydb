#pragma once

#include <stdatomic.h>

#define ATOMIC_CAS_WEAK atomic_compare_exchange_weak_explicit

#include "queue.h" // IWYU pragma: export
