#include <telly.h>

struct KVPair *get_data(Database *database, string_t key) {
  const uint64_t capacity = database->size.capacity;
  const uint64_t start_idx = (OPENSSL_LH_strhash(key.value) % capacity);
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
