#include <telly.h>

#include <string.h>

struct KVPair *get_data(struct Database *database, string_t key) {
  const uint64_t index = (hash(key.value, key.len) % database->size.capacity);
  
  for (uint64_t i = index; i < database->size.capacity; ++i) {
    struct KVPair *pair = database->data[index];

    if (!pair || ((pair->hashed % database->size.capacity) != index)) {
      return NULL;
    }

    if ((key.len == pair->key.len) && (memcmp(key.value, pair->key.value, key.len) == 0)) {
      return pair;
    }
  }

  return NULL;
}
