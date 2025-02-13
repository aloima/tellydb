// Includes Redis serializations protocol definitions and its methods

#pragma once

#include "server.h"
#include "utils.h"

#define RDT_SSTRING '+'
#define RDT_BSTRING '$'
#define RDT_ARRAY '*'
#define RDT_INTEGER ':'
#define RDT_ERR '-'
#define RDT_CLOSE 0

typedef struct CommandData {
  string_t name;
  string_t *args;
  uint32_t arg_count;
  bool close;
} commanddata_t;

commanddata_t *get_command_data(struct Client *client);
void free_command_data(commanddata_t *data);
