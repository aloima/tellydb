#include <telly.h>

#include <stddef.h>
#include <stdint.h>

#include <gmp.h>

string_t write_value(void *value, const enum TellyTypes type, const enum ProtocolVersion protover, char *buffer) {
  switch (type) {
    case TELLY_NULL:
      switch (protover) {
        case RESP2:
          return CREATE_STRING("$-1\r\n", 5);

        case RESP3:
          return CREATE_STRING("_\r\n", 3);
      }

    case TELLY_INT: {
      mpz_t *number = value;
      const size_t nbytes = create_resp_integer_mpz(protover, buffer, *number);
      return CREATE_STRING(buffer, nbytes);
    }

    case TELLY_DOUBLE: {
      mpf_t *number = value;
      const size_t nbytes = create_resp_integer_mpf(protover, buffer, *number);
      return CREATE_STRING(buffer, nbytes);
    }

    case TELLY_STR: {
      const string_t *string = value;
      const size_t nbytes = create_resp_string(buffer, *string);
      return CREATE_STRING(buffer, nbytes);
    }

    case TELLY_BOOL: {
      const bool is_true = *((bool *) value);

      switch (protover) {
        case RESP2:
          if (is_true) {
            return RESP_OK_MESSAGE("true");
          } else {
            return RESP_OK_MESSAGE("false");
          }

        case RESP3:
          if (is_true) {
            return CREATE_STRING("#t\r\n", 4);
          } else {
            return CREATE_STRING("#f\r\n", 4);
          }
      }
    }

    case TELLY_HASHTABLE:
      return RESP_OK_MESSAGE("hash table");

    case TELLY_LIST:
      return RESP_OK_MESSAGE("list");
  }
}

inline __attribute__((always_inline)) int _read(Client *client, char *buf, const size_t nbytes) {
  return (!client->ssl ? read(client->connfd, buf, nbytes) : SSL_read(client->ssl, buf, nbytes));
}

inline __attribute__((always_inline)) int _write(Client *client, char *buf, const size_t nbytes) {
  return (!client->ssl ? write(client->connfd, buf, nbytes) : SSL_write(client->ssl, buf, nbytes));
}
