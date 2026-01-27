#include <telly.h>

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#include <unistd.h>

#include <openssl/ssl.h>
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

int read_from_socket(Client *client, char *buf, const size_t nbytes) {
  size_t read_bytes = 0;

  if (client->ssl) {
    while (read_bytes < nbytes) {
      const int n = SSL_read(client->ssl, buf + read_bytes, (size_t) (nbytes - read_bytes));

      if (n <= 0) {
        const int err = SSL_get_error(client->ssl, n);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;

        return -1;
      }

      read_bytes += n;
    }
  } else {
    while (read_bytes < nbytes) {
      const int n = read(client->connfd, buf + read_bytes, (size_t) (nbytes - read_bytes));

      if (n <= 0) {
        if (errno == EINTR) continue;
        return -1;
      }

      read_bytes += n;
    }
  }

  return read_bytes;
}

int write_to_socket(Client *client, char *buf, const size_t nbytes) {
  size_t written = 0;

  if (client->ssl) {
    while (written < nbytes) {
      const int n = SSL_write(client->ssl, buf + written, (size_t) (nbytes - written));

      if (n <= 0) {
        const int err = SSL_get_error(client->ssl, n);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;

        return -1;
      }

      written += n;
    }
  } else {
    while (written < nbytes) {
      const int n = write(client->connfd, buf + written, (size_t) (nbytes - written));

      if (n <= 0) {
        if (errno == EINTR) continue;
        return -1;
      }

      written += n;
    }
  }

  return written;
}
