#pragma once

#include "server/client.h"
#include "utils/string.h"

enum IOOpType : uint8_t {
  IOOP_GET_COMMAND,
  IOOP_TERMINATE,
  IOOP_WRITE
};

void add_io_request(const enum IOOpType type, Client *client, string_t write_str);
