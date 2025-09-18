#include <telly.h>

#include <stdint.h>
#include <stdbool.h>

#include <gmp.h>

uint8_t create_resp_integer(char *buf, uint64_t value) {
  *(buf) = RDT_INTEGER;
  const uint64_t nbytes = ltoa(value, buf + 1);
  *(buf + nbytes + 1) = '\r';
  *(buf + nbytes + 2) = '\n';
  return (nbytes + 3);
}

uint64_t create_resp_integer_mpz(const enum ProtocolVersion protover, char *buf, mpz_t value) {
  uint64_t nbytes = 1;

  if (mpz_fits_slong_p(value) != 0) {
    *(buf) = RDT_INTEGER;
  } else {
    switch (protover) {
      case RESP2: {
        char *str = mpz_get_str(NULL, 10, value);
        uint64_t size = mpz_sizeinbase(value, 10);

        if (mpz_sgn(value) == -1) {
          size += 1;
        }

        const uint64_t length = ((str[size - 2] == '\0') ? (size - 1) : size);

        char digits[4]; // the maximum number of digits can be 310 (including negative sign)
        const uint8_t digit_len = ltoa(length, digits);

        *(buf) = RDT_BSTRING;
        memcpy(buf + 1, digits, digit_len);
        nbytes += digit_len;

        *(buf + nbytes++) = '\r';
        *(buf + nbytes++) = '\n';

        memcpy(buf + nbytes, str, length);
        nbytes += length;
        free(str);

        *(buf + nbytes++) = '\r';
        *(buf + nbytes++) = '\n';

        return nbytes;
      }

      case RESP3:
        *(buf) = RDT_BIGNUMBER;
        break;
    }
  }

  mpz_get_str((buf + 1), 10, value);

  nbytes += ({
    uint64_t size = mpz_sizeinbase(value, 10);

    if (mpz_sgn(value) == -1) {
      size += 1;
    }

    ((buf[size] == '\0') ? (size - 1) : size);
  });

  *(buf + nbytes++) = '\r';
  *(buf + nbytes++) = '\n';

  return nbytes;
}

uint64_t create_resp_integer_mpf(const enum ProtocolVersion protover, char *buf, mpf_t value) {
  int nbytes = 1;

  mp_exp_t exp;
  char *str = mpf_get_str(NULL, &exp, 10, 0, value);
  const uint64_t len = strlen(str);

  if (len == 0) {
    *(buf) = RDT_INTEGER;
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
  }

  if (len == exp) { // if value is in double type, but it is integer like 4.000000
    nbytes = 1;

    if (mpf_fits_slong_p(value) == 0) {
      const uint64_t length = exp;

      switch (protover) {
        case RESP2: {
          char digits[4]; // the maximum number of digits can be 310 (including negative sign)
          const uint8_t digit_len = ltoa(length, digits);

          *(buf) = RDT_BSTRING;
          memcpy(buf + 1, digits, digit_len);
          nbytes += digit_len;

          *(buf + nbytes++) = '\r';
          *(buf + nbytes++) = '\n';
          break;
        }

        case RESP3:
          *(buf) = RDT_BIGNUMBER;
          break;
      }

      memcpy(buf + nbytes, str, length);
      nbytes += length;
    } else {
      *(buf) = RDT_INTEGER;
    }
  } else {
    switch (protover) {
      case RESP2:
        if (exp == 0) {
          nbytes = 2;
          *(buf) = RDT_INTEGER;
          *(buf + 1) = '0';
        } else {
          nbytes = (((exp + 1) < nbytes) ? (exp + 1) : nbytes);
          *(buf) = RDT_INTEGER;
        }

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

uint64_t create_resp_string(char *buf, string_t string) {
  *(buf) = RDT_BSTRING;
  const uint64_t nbytes = ltoa(string.len, buf + 1);
  *(buf + nbytes + 1) = '\r';
  *(buf + nbytes + 2) = '\n';

  memcpy(buf + nbytes + 3, string.value, string.len);
  *(buf + nbytes + string.len + 3) = '\r';
  *(buf + nbytes + string.len + 4) = '\n';
  return (nbytes + string.len + 5);
}
