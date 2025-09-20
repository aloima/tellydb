#include <telly.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

bool delete_data(struct Database *database, const string_t key) {
  const uint64_t capacity = database->size.capacity;
  const uint64_t index = (hash((char *) key.value, key.len) % capacity);
  struct KVPair *kv = database->data[index];

  if (!kv) {
    return false;
  }

  free_kv(kv);

  for (uint64_t i = (index + 1); i < capacity; ++i) {
    struct KVPair *move = database->data[i];

    if (!move) {
      database->size.stored -= 1;
      database->data[i - 1] = NULL;
      return true;
    }

    if (index == (move->hashed % capacity)) {
      database->data[i - 1] = move;
    }
  }

  return true;
}
