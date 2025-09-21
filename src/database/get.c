#include <telly.h>

#include <stddef.h>
#include <string.h>

static inline uint64_t add_to_index(const uint64_t index, const uint64_t capacity) {
  return ((index + 1) % capacity);
}

struct KVPair *get_data(struct Database *database, string_t key) {
  const uint64_t capacity = database->size.capacity;
  const uint64_t start_idx = ((hash(key.value, key.len) % capacity));
  uint64_t index = start_idx;

  struct KVPair *pair;

  while ((pair = database->data[index])) {
    if ((pair->hashed % capacity) != index) {
      break;
    }

    if ((key.len == pair->key.len) && (memcmp(key.value, pair->key.value, key.len) == 0)) {
      return pair;
    }

    index = add_to_index(index, capacity);

    if (index == start_idx) {
      break;
    }
  }

  return NULL;
}
