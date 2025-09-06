#include <telly.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

string_t write_value(void *value, const enum TellyTypes type, const enum ProtocolVersion protover, char *buffer) {
  switch (type) {
    case TELLY_NULL:
      return RESP_OK_MESSAGE("null");

    case TELLY_NUM: {
      const size_t nbytes = create_resp_integer(buffer, (*(long *) value));
      return CREATE_STRING(buffer, nbytes);
    }

    case TELLY_STR: {
      const string_t *string = value;

      const size_t nbytes = create_resp_string(buffer, *string);
      return CREATE_STRING(buffer, nbytes);
    }

    case TELLY_BOOL:
      if (*((bool *) value)) {
        if (protover == RESP3) {
          return CREATE_STRING("#t\r\n", 4);
        } else {
          return RESP_OK_MESSAGE("true");
        }
      } else {
        if (protover == RESP3) {
          return CREATE_STRING("#f\r\n", 4);
        } else {
          return RESP_OK_MESSAGE("false");
        }
      }

    case TELLY_HASHTABLE:
      return RESP_OK_MESSAGE("hash table");

    case TELLY_LIST:
      return RESP_OK_MESSAGE("list");
  }
}
