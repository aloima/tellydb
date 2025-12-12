#include <telly.h>

#include <stdint.h>
#include <inttypes.h>

int32_t take_n_bytes_from_socket(Client *client, char *buf, int32_t *at, char **data, const uint32_t n, int32_t *size) {
  // Once dereferencing and set as varaible of each cost > Once dereferencing cost of each
  // const int32_t current_end = *end; 

  const int32_t current_at = *at; 

  const uint32_t remaining = (*size - current_at);
  *data = (buf + current_at);

  if (VERY_LIKELY(n <= remaining)) {
    *at = current_at + n;
    return n;
  }

  if (_read(client, *data + remaining, n - remaining) <= 0) {
    return remaining;
  }

  // Needs to read buffer, because the process may be interrupted in the middle of getting command data
  *size = _read(client, buf, RESP_BUF_SIZE);
  *at = 0;
  return n;
}
