#include "../../headers/telly.h"

#include <stddef.h>
#include <stdlib.h>

struct KVPair *set_data(struct KVPair kv, struct Configuration *conf) {
  struct BTree *cache = get_cache();
  struct KVPair *data = get_data(kv.key.value, conf);

  if (data != NULL) {
    if (data->type == TELLY_STR && kv.type != TELLY_STR) {
      free(data->value.string.value);
    } else if (data->type == TELLY_HASHTABLE && kv.type != TELLY_HASHTABLE) {
      free_hashtable(data->value.hashtable);
    } else if (data->type == TELLY_LIST && kv.type != TELLY_LIST) {
      free_list(data->value.list);
    }

    switch (kv.type) {
      case TELLY_STR:
        set_string(&data->value.string, kv.value.string.value, kv.value.string.len, data->value.string.value == NULL);
        break;

      case TELLY_INT:
        data->value.integer = kv.value.integer;
        break;

      case TELLY_BOOL:
        data->value.boolean = kv.value.boolean;
        break;

      case TELLY_HASHTABLE:
        data->value.hashtable = kv.value.hashtable;
        break;

      case TELLY_LIST:
        data->value.list = kv.value.list;
        break;

      case TELLY_NULL:
        data->value.null = NULL;
        break;
    }

    data->type = kv.type;
  } else {
    void *value = get_kv_val(&kv, kv.type);
    struct KVPair *data = insert_kv_to_btree(cache, kv.key.value, value, kv.type);
    data->pos = -1;
  }

  return data;
}
