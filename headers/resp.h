// Includes Redis serializations protocol definitions and its methods

#pragma once

#include "server.h"
#include "utils.h"

#include <stdbool.h>
#include <stdint.h>

#define RDT_SSTRING '+'
#define RDT_BSTRING '$'
#define RDT_ARRAY '*'
#define RDT_INTEGER ':'
#define RDT_ERR '-'
#define RDT_CLOSE 0

#define RESP_BUF_SIZE 4096

#define CREATE_RESP_INTEGER_VARIABLED(buf, value, nbytes) \
  *buf = ':'; \
  nbytes = ltoa(value, buf + 1); \
  *(buf + nbytes + 1) = '\r'; \
  *(buf + nbytes + 2) = '\n';

#define CREATE_RESP_INTEGER(buf, value) \
  *buf = ':'; \
  const int __nbytes = ltoa(value, buf + 1); \
  *(buf + __nbytes + 1) = '\r'; \
  *(buf + __nbytes + 2) = '\n';

typedef struct CommandData {
  string_t name;

  string_t *args;
  uint32_t arg_count;
} commanddata_t;

bool get_command_data(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command);
void free_command_data(commanddata_t data);
