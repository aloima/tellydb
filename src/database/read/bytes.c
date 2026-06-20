#include <telly.h>
#include "read.h"

void collect_bytes(const GenericArguments *arguments, const uint32_t count, void *data) {
  auto fd = arguments->fd;
  auto block = arguments->block;
  auto block_size = arguments->block_size;
  auto at = arguments->at;

  uint32_t remaining = count;

  if ((*at + remaining) < block_size) {
    ASSERT(memcpy(data, block + *at, remaining), !=, NULL);
    *at += remaining;
    return;
  }

  const uint16_t available = (block_size - *at);
  ASSERT(memcpy(data + (count - remaining), block + *at, available), !=, NULL);
  ASSERT(read(fd, block, block_size), ==, (int64_t) block_size);
  remaining -= available;

  while (remaining >= block_size) {
    ASSERT(memcpy(data + (count - remaining), block, block_size), !=, NULL);
    ASSERT(read(fd, block, block_size), ==, (int64_t) block_size);
    remaining -= block_size;
  }

  ASSERT(memcpy(data + (count - remaining), block, remaining), !=, NULL);
  *at = remaining;
}