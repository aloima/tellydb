#include <telly.h>

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
    set_kv(kv, key, value, type);

    // TODO: implement
    return NULL;
  }
}
