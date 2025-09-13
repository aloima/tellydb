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

#define RDT_BIGNUMBER '('
#define RDT_BIGNUMBER_SL "("

#define RDT_DOUBLE ','
#define RDT_DOUBLE_SL ","

#define RDT_ERR '-'
#define RDT_ERR_SL "-"

#define RESP_BUF_SIZE 4096
#define COMMAND_NAME_MAX_LENGTH 64

int create_resp_integer(char *buf, uint64_t value);
int create_resp_integer_mpf(const enum ProtocolVersion protover, char *buf, mpf_t value);
uint64_t create_resp_string(char *buf, string_t string);

typedef struct {
  char value[COMMAND_NAME_MAX_LENGTH];
  uint32_t len;
} commandname_t;

typedef struct CommandData {
  commandname_t name;

  string_t *args;
  uint32_t arg_count;
} commanddata_t;

bool get_command_data(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command);
void free_command_data(commanddata_t data);
