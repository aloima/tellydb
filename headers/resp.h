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

typedef struct CommandData {
  string_t name;

  string_t *args;
  uint32_t arg_count;
} commanddata_t;

bool get_command_data(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command);
void free_command_data(commanddata_t data);
