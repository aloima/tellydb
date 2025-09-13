#include <telly.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <gmp.h>

uint8_t create_resp_integer(char *buf, uint64_t value) {
  *(buf) = RDT_INTEGER;
  const uint64_t nbytes = ltoa(value, buf + 1);
  *(buf + nbytes + 1) = '\r';
  *(buf + nbytes + 2) = '\n';
  return (nbytes + 3);
}

uint64_t create_resp_integer_mpf(const enum ProtocolVersion protover, char *buf, mpf_t value) {
  int nbytes = 1;

  mp_exp_t exp;
  char *str = mpf_get_str(NULL, &exp, 10, 0, value);
  const uint64_t len = strlen(str);
  bool zeroed = true;

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

    if (exp < i && zeroed && str[i] != 0) {
      zeroed = false;
    }
  }

  if (zeroed) {
    nbytes = ((nbytes < exp) ? nbytes : exp);

    if (mpf_fits_slong_p(value) == 0) {
      switch (protover) {
        case RESP2:
          // TODO: string representation
          *(buf) = RDT_INTEGER;
          break;

        case RESP3:
          *(buf) = RDT_BIGNUMBER;
          break;
      }
    } else {
      *(buf) = RDT_INTEGER;
    }
  } else {
    switch (protover) {
      case RESP2:
        nbytes = ((nbytes < exp) ? nbytes : exp);
        *(buf) = RDT_INTEGER;
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
