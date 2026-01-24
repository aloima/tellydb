#include <telly.h>

#include <stdint.h>
#include <stdlib.h>

#include <gmp.h>

static inline uint64_t probe(struct KVPair **data, uint64_t index, const uint64_t capacity) {
  while (data[index]) index = (index + 1) % capacity;
  return index;
}

static inline bool extend_database(Database *database, const uint64_t old_capacity) {
  struct KVPair **nd = calloc(old_capacity * 2, sizeof(struct KVPair *));
  if (!nd) return false;

  database->size.capacity *= 2;
  const uint64_t new_capacity = database->size.capacity;

  for (uint64_t i = 0; i < old_capacity; ++i) {
    struct KVPair *current = database->data[i];

    if (current) {
      const uint64_t index = (current->hashed % new_capacity);
      nd[probe(nd, index, new_capacity)] = current;
    }
  }

  free(database->data);
  database->data = nd;
  return true;
}

struct KVPair *set_data(Database *database, struct KVPair *data, const string_t key, void *value, const enum TellyTypes type) {
  if (data) {
    free_value(data->type, data->value);
    data->type = type;
    data->value = value;

    return data;
  }

  const uint64_t old_capacity = database->size.capacity;

  // If 75% of table or more is filled
  if ((old_capacity * 3) <= (database->size.stored * 4) && !extend_database(database, old_capacity))
    return NULL;

  struct KVPair *kv = malloc(sizeof(struct KVPair));
  if (!kv) return NULL;

  set_kv(kv, key, value, type);

  const uint64_t new_capacity = database->size.capacity;
  const uint64_t index = (kv->hashed % new_capacity);

  database->data[probe(database->data, index, new_capacity)] = kv;
  database->size.stored += 1;

  return kv;
}
