#include "../../headers/server.h"
#include "../../headers/utils.h"

#include <openssl/ssl.h>

#include <unistd.h>

ssize_t _read(struct Client *client, void *buf, const size_t nbytes) {
  if (client->ssl) {
    return SSL_read(client->ssl, buf, nbytes);
  } else {
    return read(client->connfd, buf, nbytes);
  }
}

ssize_t _write(struct Client *client, void *buf, const size_t nbytes) {
  if (client->ssl) {
    return SSL_write(client->ssl, buf, nbytes);
  } else {
    return write(client->connfd, buf, nbytes);
  }
}

void write_value(struct Client *client, value_t value, enum TellyTypes type) {
  switch (type) {
    case TELLY_NULL:
      _write(client, "+null\r\n", 7);
      break;

    case TELLY_INT: {
      const uint32_t digit_count = get_digit_count(value.integer);
      const uint32_t buf_len = digit_count + 3;

      char buf[buf_len + 1];
      sprintf(buf, ":%d\r\n", value.integer);

      _write(client, buf, buf_len);
      break;
    }

    case TELLY_STR: {
      const uint32_t buf_len = get_digit_count(value.string.len) + value.string.len + 5;

      char buf[buf_len + 1];
      sprintf(buf, "$%ld\r\n%s\r\n", value.string.len, value.string.value);

      _write(client, buf, buf_len);
      break;
    }

    case TELLY_BOOL:
      if (value.boolean) {
        _write(client, "+true\r\n", 7);
      } else {
        _write(client, "+false\r\n", 8);
      }

      break;

    case TELLY_HASHTABLE:
      _write(client, "+hash table\r\n", 13);
      break;

    case TELLY_LIST:
      _write(client, "+list\r\n", 7);
      break;

    default:
      break;
  }
}
