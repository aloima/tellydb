#include "utils.h"

#include <stdbool.h>
#include <stdint.h>

#ifndef RESP_H
  #define RESP_H

  #define RDT_SSTRING '+'
  #define RDT_BSTRING '$'
  #define RDT_ARRAY '*'
  #define RDT_INTEGER ':'
  #define RDT_ERR '-'

  typedef struct RESPData {
    uint8_t type;

    union {
      string_t string;
      bool boolean;
      int32_t integer;
      double doubl;
      struct RESPData *array;
    } value;

    uint32_t count;
  } respdata_t;

  respdata_t get_resp_data(int connfd);
#endif
