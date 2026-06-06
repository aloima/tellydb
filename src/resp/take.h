#pragma once

#include <telly.h>

__attribute__((always_inline)) inline
int32_t take_n_bytes(Client *client, int32_t *at, char **data, const uint32_t n, int32_t *size) {
  // "Once dereferencing and set as variable of each" cost > "Once dereferencing" cost of each
  const int32_t current_at = *at;
  const uint32_t remaining = (*size - current_at);
  *data = (client->read_buf->data + current_at);

  if (VERY_LIKELY(n <= remaining)) {
    *at = current_at + n;
    return n;
  }

  const uint32_t required = *size + (n - remaining);

  if (required > client->read_buf->size) {
    client->read_buf->data = realloc(client->read_buf->data, required);
    if (client->read_buf->data == NULL)
      return -1;

    client->read_buf->size = required;
    *data = (client->read_buf->data + current_at);
  }

  if (read_from_socket(client, *data + remaining, n - remaining) <= 0) {
    return remaining;
  }

  /*
   // Needs to read buffer, because the process may be interrupted in the middle of getting command data
   *size = read_from_socket(client, buf, RESP_BUF_SIZE);
   *at = 0;
  */
  *size += (n - remaining);
  *at = current_at + n;
  return n;
}
