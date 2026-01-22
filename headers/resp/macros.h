#pragma once

#include "../server/client.h" // IWYU pragma: export
#include "../utils/string.h"
#include "types.h"

#define RESP_NULL(protover) ({\
  string_t response;\
  switch (protover) {\
    case RESP2:\
      response = CREATE_STRING("$-1\r\n", 5);\
      break;\
\
    case RESP3:\
      response = CREATE_STRING("_\r\n", 3);\
      break;\
  }\
\
  response;\
})

#define RESP_OK()                   CREATE_STRING(RDT_SSTRING "OK\r\n",       5)
#define RESP_OK_MESSAGE(message)    CREATE_STRING(RDT_SSTRING message "\r\n", sizeof(message) + 2)
#define RESP_ERROR()                CREATE_STRING(RDT_ERROR   "ERROR\r\n",    8)
#define RESP_ERROR_MESSAGE(message) CREATE_STRING(RDT_ERROR   message "\r\n", sizeof(message) + 2)
