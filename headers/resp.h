#pragma once

#include "server/server.h"
#include "utils.h"

#include <stdlib.h>
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

static inline int create_resp_integer(char *buf, uint64_t value) {
  *(buf) = RDT_INTEGER;
  const uint64_t nbytes = ltoa(value, buf + 1);
  *(buf + nbytes + 1) = '\r';
  *(buf + nbytes + 2) = '\n';
  return (nbytes + 3);
}

// TODO: "Invalid response type 13" from redis-cli on RDT_BIGNUMBER
static inline int create_resp_integer_mpf(const enum ProtocolVersion protover, char *buf, mpf_t value) {
  int nbytes = 1;

  mp_exp_t exp;
  char *str = mpf_get_str(NULL, &exp, 10, 0, value);
  const uint64_t len = strlen(str);
  bool zeroed = true;

  if (len == 0) {
    switch (protover) {
      case RESP2:
        *(buf) = RDT_INTEGER;
        break;

      case RESP3:
        *(buf) = RDT_BIGNUMBER;
        break;
    }

    *(buf + 1) = '0';
    *(buf + 2) = '\r';
    *(buf + 3) = '\n';
    return 4;
  }

  if (str[0] == '-') {
    exp += 1;
  }

  for (uint64_t i = 0; i < len; ++i) {
    if (i == exp) {
      buf[nbytes++] = '.';
    }

    buf[nbytes++] = str[i];

    if (exp < i && zeroed && str[i] != 0) {
      zeroed = false;
    }
  }

  if (zeroed) {
    nbytes = ((nbytes < exp) ? nbytes : exp);

    if (mpf_fits_slong_p(value) == 0) {
      switch (protover) {
        case RESP2:
          // TODO: string representation
          break;

        case RESP3:
          *(buf) = RDT_BIGNUMBER;
          break;
      }
    } else {
      *(buf) = RDT_INTEGER;
    }
  } else {
    switch (protover) {
      case RESP2:
        nbytes = ((nbytes < exp) ? nbytes : exp);
        *(buf) = RDT_INTEGER;
        break;

      case RESP3:
        *(buf) = RDT_DOUBLE;
        break;
    }
  }

  *(buf + nbytes++) = '\r';
  *(buf + nbytes++) = '\n';
  free(str);

  return nbytes;
}

static inline uint64_t create_resp_string(char *buf, string_t string) {
  *(buf) = RDT_BSTRING;
  const uint64_t nbytes = ltoa(string.len, buf + 1);
  *(buf + nbytes + 1) = '\r';
  *(buf + nbytes + 2) = '\n';

  memcpy(buf + nbytes + 3, string.value, string.len);
  *(buf + nbytes + string.len + 3) = '\r';
  *(buf + nbytes + string.len + 4) = '\n';
  return (nbytes + string.len + 5);
}

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
