#include <telly.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

static inline uint64_t add_to_index(const uint64_t index, const uint64_t capacity) {
  return ((index + 1) % capacity);
}

bool delete_data(struct Database *database, const string_t key) {
  const uint64_t capacity = database->size.capacity;
  const uint64_t start_idx = (hash((char *) key.value, key.len) % capacity);
  uint64_t index = index;
  struct KVPair *kv;

  while ((kv = database->data[index])) {
    if ((kv->hashed % capacity) != index) {
      return false;
    } else if ((kv->key.len == key.len) && (memcmp(kv->key.value, key.value, key.len) == 0)) {
      break;
    }

    index = add_to_index(index, capacity);

    if (index == start_idx) {
      return false;
    }
  }

  free_kv(kv);
  database->data[index] = NULL; // Needs it for uncollised indexes and filled next index

  for (uint64_t i = add_to_index(index, capacity); i != index; i = add_to_index(index, capacity)) {
    struct KVPair *pair = database->data[i];
    const uint64_t prev = ((i == 0) ? (capacity - 1) : (i - 1));

    if (!pair) {
      database->size.stored -= 1;
      database->data[prev] = NULL;
      break;
    }

    // On collised index
    if (index == (pair->hashed % capacity)) {
      database->data[prev] = pair;
    } else {
      break;
    }
  }

  return true;
}
