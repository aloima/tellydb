#include <telly.h>

#include <stdint.h>
#include <inttypes.h>

int32_t take_n_bytes_from_socket(struct Client *client, char *buf, int32_t *at, char **data, const uint32_t n, int32_t *size) {
  const uint32_t remaining = (*size - *at);

  if (VERY_LIKELY(n <= remaining)) {
    if (data != NULL) {
      *data = (buf + *at);
    }

    *at += n;
    return n;
  }

  if (data != NULL) {
    *data = (buf + *at);

    if (_read(client, *data + remaining, n - remaining) <= 0) {
      return remaining;
    }

    // Needs to read buffer, because the process may be interrupted in the middle of getting command data
    *size = _read(client, buf, RESP_BUF_SIZE);

    if (*size == -1) {
      return -1;
    }
  } else {
    char dummy[128];
    const uint32_t length = (n - remaining);
    const uint32_t quotient = (length / sizeof(dummy));
    const uint32_t rest = (length % sizeof(dummy));
    uint32_t total = remaining;

    for (uint32_t i = 0; i < quotient; ++i) {
      int got = _read(client, dummy, sizeof(dummy));

      if (got <= 0) {
        return total + (got > 0 ? got : 0);
      }

      total += got;

      if ((uint32_t) got < sizeof(dummy)) {
        return total;
      }
    }

    if (rest > 0) {
      int got = _read(client, dummy, rest);

      if (got <= 0) {
        return total + (got > 0 ? got : 0);
      }

      total += got;

      if ((uint32_t) got < rest) {
        return total;
      }
    }

    *size = _read(client, buf, RESP_BUF_SIZE);

    if (*size <= 0) {
      return total;
    }
  }

  *at = 0;
  return n;
}

