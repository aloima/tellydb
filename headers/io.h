#pragma once

#include "server/_client.h"

#include <stdint.h>

#include <pthread.h>

enum IOOpType : uint8_t {
  IOOP_GET_COMMAND,
  IOOP_WRITE
};

void create_io_threads(const uint32_t count);
void add_io_request(const enum IOOpType type, struct Client *client);
void destroy_io_threads();
