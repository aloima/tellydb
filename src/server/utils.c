#include "../../headers/telly.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void write_value(struct Client *client, void *value, enum TellyTypes type) {
  switch (type) {
    case TELLY_NULL:
      _write(client, "+null\r\n", 7);
      break;

    case TELLY_NUM: {
      char buf[24];
      const size_t nbytes = sprintf(buf, ":%ld\r\n", *((long *) value));
      _write(client, buf, nbytes);
      break;
    }

    case TELLY_STR: {
      const string_t *string = value;

      char *buf = malloc(26 + string->len);
      const size_t nbytes = sprintf(buf, "$%d\r\n%.*s\r\n", string->len, string->len, string->value);
      _write(client, buf, nbytes);
      free(buf);
      break;
    }

    case TELLY_BOOL:
      if (*((bool *) value)) {
        if(client->protover == RESP3) _write(client, "#t\r\n", 4);
        else _write(client, "+true\r\n", 7);
      } else {
        if(client->protover == RESP3) _write(client, "#f\r\n", 4);
        else _write(client, "+false\r\n", 8);
      }

      break;

    case TELLY_HASHTABLE:
      _write(client, "+hash table\r\n", 13);
      break;

    case TELLY_LIST:
      _write(client, "+list\r\n", 7);
      break;
  }
}
