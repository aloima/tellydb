#pragma once

#include "../server/client.h"
#include "../utils/arena.h"
#include "../utils/utils.h"

#include <stdbool.h>
#include <stdint.h>

#include <gmp.h>

#define INITIAL_RESP_ARENA_SIZE 65536

#define COMMAND_NAME_MAX_LENGTH 64
#define RESP_INLINE_BUFFER 128
#define RESP_INLINE_ARGUMENT_COUNT 32

typedef struct CommandArgs {
  string_t *data;
  uint32_t count;
} commandargs_t;

typedef struct CommandData {
  string_t *name;
  commandargs_t args;
} commanddata_t;

bool parse_resp_command(Arena *arena, Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *cmd);
bool parse_inline_command(Arena *arena, Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *cmd, char c);
bool get_command_data(Arena *arena, Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command);

#include "macros.h" // IWYU pragma: export
#include "types.h"  // IWYU pragma: export
