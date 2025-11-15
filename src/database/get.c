#include <telly.h>

#include <stddef.h>
#include <string.h>

struct KVPair *get_data(struct Database *database, string_t key) {
  const uint64_t capacity = database->size.capacity;
  const uint64_t start_idx = (hash(key.value, key.len) % capacity);
  uint64_t index = start_idx;

  struct KVPair *pair;

  while ((pair = database->data[index])) {
    if ((key.len == pair->key.len) && (memcmp(key.value, pair->key.value, key.len) == 0))
      return pair;

    index = ((index + 1) % capacity);
    if (index == start_idx) break;
  }

  return NULL;
}
