#pragma once

#include "take.h"          // IWYU pragma: export
#include "utils/logging.h" // IWYU pragma: export

#define THROW_RESP_ERROR(id) \
  do { \
    write_log(LOG_ERR, "Received data from Client #%" PRIu32 " cannot be validated as a RESP data.", id); \
    return false; \
  } while (0)

#define TAKE_BYTES(value, n, return_value) \
  if (VERY_UNLIKELY(take_n_bytes(client, at, (char **) &value, n, size) != (int32_t) n)) { \
    write_log(LOG_ERR, "Received data from Client #%" PRIu32 " cannot be validated as a RESP data.", client->id); \
    return return_value; \
  }
