#include <telly.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

bool delete_data(struct Database *database, const string_t key) {
  const uint64_t capacity = database->size.capacity;
  uint64_t index = (hash((char *) key.value, key.len) % capacity);
  struct KVPair *kv;

  while ((kv = database->data[index])) {
    if ((kv->hashed % capacity) != index) {
      return false;
    } else if ((kv->key.len == key.len) && (memcmp(kv->key.value, key.value, key.len) == 0)) {
      break;
    }

    index += 1;

    if (index == capacity) {
      return false;
    }
  }

  if (!kv) {
    return false;
  }

  free_kv(kv);

  for (uint64_t i = (index + 1); i < capacity; ++i) {
    struct KVPair *pair = database->data[i];

    if (!pair) {
      database->size.stored -= 1;
      database->data[i - 1] = NULL;
      return true;
    }

    if (index == (pair->hashed % capacity)) {
      database->data[i - 1] = pair;
    }
  }

  return false;
}
