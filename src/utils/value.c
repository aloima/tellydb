#include "../../headers/utils.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>

void write_value(int connfd, value_t value, enum TellyTypes type) {
  switch (type) {
    case TELLY_NULL:
      write(connfd, "+null\r\n", 7);
      break;

    case TELLY_INT: {
      const uint32_t digit_count = get_digit_count(value.integer);
      const uint32_t buf_len = digit_count + 3;

      char buf[buf_len + 1];
      sprintf(buf, ":%d\r\n", value.integer);

      write(connfd, buf, buf_len);
      break;
    }

    case TELLY_STR: {
      const uint32_t buf_len = get_digit_count(value.string.len) + value.string.len + 5;

      char buf[buf_len + 1];
      sprintf(buf, "$%ld\r\n%s\r\n", value.string.len, value.string.value);

      write(connfd, buf, buf_len);
      break;
    }

    case TELLY_BOOL:
      if (value.boolean) {
        write(connfd, "+true\r\n", 7);
      } else {
        write(connfd, "+false\r\n", 8);
      }

      break;

    case TELLY_HASHTABLE:
      write(connfd, "+a hash table\r\n", 15);
      break;

    case TELLY_LIST:
      write(connfd, "+a list\r\n", 9);
      break;

    default:
      break;
  }
}
