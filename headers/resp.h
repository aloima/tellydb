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

static inline int create_resp_integer(char *buf, uint64_t value) {
  *(buf) = ':';
  const uint64_t __nbytes = ltoa(value, buf + 1);
  *(buf + __nbytes + 1) = '\r';
  *(buf + __nbytes + 2) = '\n';
  return (__nbytes + 3);
}

static inline uint64_t create_resp_string(char *buf, string_t string) {
  *(buf) = '$';
  const uint64_t __nbytes = ltoa(string.len, buf + 1);
  *(buf + __nbytes + 1) = '\r';
  *(buf + __nbytes + 2) = '\n';

  memcpy(buf + __nbytes + 2, string.value, string.len);
  *(buf + __nbytes + string.len + 2) = '\r';
  *(buf + __nbytes + string.len + 3) = '\n';
  return (__nbytes + string.len + 4);
}

typedef struct CommandData {
  string_t name;

  string_t *args;
  uint32_t arg_count;
} commanddata_t;

bool get_command_data(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command);
void free_command_data(commanddata_t data);
