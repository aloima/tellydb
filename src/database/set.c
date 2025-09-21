#include <telly.h>

#include <stdint.h>
#include <stdlib.h>

#include <gmp.h>

struct KVPair *set_data(struct Database *database, struct KVPair *data, const string_t key, void *value, const enum TellyTypes type) {
  if (data) {
    free_value(data->type, data->value);
    data->type = type;
    data->value = value;

    return data;
  } else {
    struct KVPair *kv = malloc(sizeof(struct KVPair));

    if (!kv) {
      return NULL;
    }

    set_kv(kv, key, value, type);

    if (database->size.capacity == database->size.stored) {
      struct KVPair **nd = calloc(database->size.capacity * 2, sizeof(struct KVPair *));

      if (!nd) {
        return NULL;
      }

      database->size.capacity *= 2;

      for (uint64_t i = 0; i < database->size.stored; ++i) {
        // No need for NULL checking, because database is fulled already, all data is available.
        struct KVPair *current = database->data[i];
        const uint64_t index = (current->hashed % database->size.capacity);

        nd[index] = current;
      }

      free(database->data);
      database->data = nd;
    }

    uint64_t index = (kv->hashed % database->size.capacity);

    while (database->data[index]) {
      index = ((index + 1) % database->size.capacity);
    }

    database->data[index] = kv;
    database->size.stored += 1;

    return kv;
  }
}
