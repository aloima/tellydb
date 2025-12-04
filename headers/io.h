#pragma once

#include "server/client.h"

#include <stdint.h>

enum IOOpType : uint8_t {
  IOOP_GET_COMMAND,
  IOOP_TERMINATE,
  IOOP_WRITE
};

int create_io_threads(const uint32_t count);
void add_io_request(const enum IOOpType type, Client *client);
void destroy_io_threads();
