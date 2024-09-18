#include "../../headers/telly.h"

#include <stddef.h>
#include <stdlib.h>

struct KVPair *set_data(struct KVPair pair, struct Configuration *conf) {
  struct BTree *cache = get_cache();
  struct KVPair *data = get_data(pair.key.value, conf);

  if (data != NULL) {
    if (data->type == TELLY_STR && pair.type != TELLY_STR) {
      free(data->value.string.value);
    } else if (data->type == TELLY_HASHTABLE && pair.type != TELLY_HASHTABLE) {
      free_hashtable(data->value.hashtable);
    }

    switch (pair.type) {
      case TELLY_STR:
        set_string(&data->value.string, pair.value.string.value, pair.value.string.len, data->value.string.value == NULL);
        break;

      case TELLY_INT:
        data->value.integer = pair.value.integer;
        break;

      case TELLY_BOOL:
        data->value.boolean = pair.value.boolean;
        break;

      case TELLY_HASHTABLE:
        data->value.hashtable = pair.value.hashtable;
        break;

      case TELLY_LIST:
        data->value.list = pair.value.list;
        break;

      case TELLY_NULL:
        data->value.null = NULL;
        break;
    }

    data->type = pair.type;
  } else {
    void *value = get_kv_val(&pair, pair.type);
    struct KVPair *data = insert_kv_to_btree(cache, pair.key.value, value, pair.type);
    data->pos = -1;
  }

  return data;
}
