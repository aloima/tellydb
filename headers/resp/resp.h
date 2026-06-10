#pragma once

#include "../server/client.h"
#include "../utils/utils.h"

#include <stdint.h>

#include <gmp.h>

#define RESP_INLINE_ARGUMENT_COUNT 32

typedef struct CommandArgs {
  string_t *data;
  uint32_t count;
} commandargs_t;

typedef struct CommandData {
  string_t name;
  commandargs_t args;
} commanddata_t;

bool parse_resp_command(Client *client, int32_t *at, int32_t *size, commanddata_t *cmd);
bool parse_inline_command(Client *client, int32_t *at, int32_t *size, commanddata_t *cmd, char c);
bool get_command_data(Client *client, int32_t *at, int32_t *size, commanddata_t *command);

#include "macros.h" // IWYU pragma: export
#include "types.h"  // IWYU pragma: export
