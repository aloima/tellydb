#include "../../headers/database.h"
#include "../../headers/hashtable.h"
#include "../../headers/btree.h"
#include "../../headers/utils.h"

#include <stdint.h>
#include <stdlib.h>

struct KVPair *set_data(struct KVPair *data, const string_t key, void *value, const enum TellyTypes type) {
  if (data) {
    switch (data->type) {
      case TELLY_STR:
        free(((string_t *) data->value)->value);
        free(data->value);
        break;

      case TELLY_HASHTABLE:
        free_hashtable(data->value);
        break;

      case TELLY_LIST:
        free_list(data->value);
        break;

      case TELLY_NULL:
        break;

      default:
        free(data->value);
        break;
    }

    data->type = type;
    data->value = value;

    return data;
  } else {
    struct BTree *cache = get_cache();
    struct KVPair *kv = malloc(sizeof(struct KVPair));
    set_kv(kv, key, value, type);

    return insert_value_to_btree(cache, hash(key.value, key.len), kv)->data;
  }
}
