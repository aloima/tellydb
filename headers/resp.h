#pragma once

#include "server/server.h"
#include "utils.h"

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

#define RESP_BUF_SIZE 4096
#define COMMAND_NAME_MAX_LENGTH 64

typedef struct {
  char value[COMMAND_NAME_MAX_LENGTH];
  uint32_t len;
} commandname_t;

typedef struct CommandData {
  commandname_t name;

  string_t *args;
  uint32_t arg_count;
} commanddata_t;

int32_t take_n_bytes_from_socket(struct Client *client, char *buf, int32_t *at, void *data, const uint32_t n, int32_t *size);

static inline void throw_resp_error(const int client_id) {
  write_log(LOG_ERR, "Received data from Client #%" PRIu32 " cannot be validated as a RESP data.", client_id);
}

#define TAKE_BYTES(value, n, return_value) \
  if (VERY_UNLIKELY(take_n_bytes_from_socket(client, buf, at, value, n, size) != n)) { \
    throw_resp_error(client->id); \
    return return_value; \
  }

bool parse_resp_command(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command);
bool parse_inline_command(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command, char c);

bool get_command_data(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command);
void free_command_data(commanddata_t data);


uint8_t create_resp_integer(char *buf, uint64_t value);
uint64_t create_resp_integer_mpf(const enum ProtocolVersion protover, char *buf, mpf_t value);
uint64_t create_resp_integer_mpz(const enum ProtocolVersion protover, char *buf, mpz_t value);
uint64_t create_resp_string(char *buf, string_t string);
