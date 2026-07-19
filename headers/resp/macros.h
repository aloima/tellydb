#pragma once

#include "../utils/string.h"
#include "types.h"

#define RESP_NULL(protover) ({                   \
  string_t response = EMPTY_STRING();            \
  switch (protover) {                            \
    case RESP2:                                  \
      response = CREATE_SIZED_STRING("$-1\r\n"); \
      break;                                     \
                                                 \
    case RESP3:                                  \
      response = CREATE_SIZED_STRING("_\r\n");   \
      break;                                     \
  }                                              \
                                                 \
  response;                                      \
})

#define RESP_OK()                   CREATE_SIZED_STRING(RDT_SSTRING "OK"    "\r\n")
#define RESP_OK_MESSAGE(message)    CREATE_SIZED_STRING(RDT_SSTRING message "\r\n")
#define RESP_ERROR()                CREATE_SIZED_STRING(RDT_ERROR   "ERROR" "\r\n")
#define RESP_ERROR_MESSAGE(message) CREATE_SIZED_STRING(RDT_ERROR   message "\r\n")
#define OUT_OF_MEMORY()             RESP_ERROR_MESSAGE("Out of memory")
