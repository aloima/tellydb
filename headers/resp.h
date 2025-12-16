#pragma once

#include "server/server.h"
#include "utils/utils.h"

#include <stdbool.h>
#include <stdint.h>

#include <gmp.h>

#define RDT_SSTRING '+'
#define RDT_SSTRING_SL "+"

#define RDT_BSTRING '$'
#define RDT_BSTRING_SL "$"

#define RDT_ARRAY '*'
#define RDT_ARRAY_SL "*"

#define RDT_INTEGER ':'
#define RDT_INTEGER_SL ":"

// valkey-cli / probably redis-cli do not support formatting/sending big numbers
// valkey-cli throws "Unknown type: 13"
#define RDT_BIGNUMBER '('
#define RDT_BIGNUMBER_SL "("

#define RDT_DOUBLE ','
#define RDT_DOUBLE_SL ","

#define RDT_ERR '-'
#define RDT_ERR_SL "-"

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

uint8_t create_resp_integer(char *buf, uint64_t value);
uint64_t create_resp_integer_mpf(const enum ProtocolVersion protover, char *buf, mpf_t value);
uint64_t create_resp_integer_mpz(const enum ProtocolVersion protover, char *buf, mpz_t value);
uint64_t create_resp_string(char *buf, string_t string);
