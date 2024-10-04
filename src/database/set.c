#include "../../headers/telly.h"

#include <stddef.h>
#include <stdlib.h>

struct KVPair *set_data(string_t key, value_t value, enum TellyTypes type, struct Configuration *conf) {
  struct BTree *cache = get_cache();
  struct KVPair *data = get_data(key.value, conf);

  if (data) {
    if (data->type == TELLY_STR && type != TELLY_STR) {
      free(data->value->string.value);
    } else if (data->type == TELLY_HASHTABLE && type != TELLY_HASHTABLE) {
      free_hashtable(data->value->hashtable);
    } else if (data->type == TELLY_LIST && type != TELLY_LIST) {
      free_list(data->value->list);
    }

    switch (type) {
      case TELLY_STR:
        set_string(&data->value->string, value.string.value, value.string.len, data->value->string.value == NULL);
        break;

      case TELLY_INT:
        data->value->integer = value.integer;
        break;

      case TELLY_BOOL:
        data->value->boolean = value.boolean;
        break;

      case TELLY_HASHTABLE:
        data->value->hashtable = value.hashtable;
        break;

      case TELLY_LIST:
        data->value->list = value.list;
        break;

      case TELLY_NULL:
        data->value->null = NULL;
        break;

      default:
        break;
    }

    data->type = type;
    return data;
  } else {
    return insert_kv_to_btree(cache, key, &value, type, -1);
  }
}
