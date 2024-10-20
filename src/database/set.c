#include "../../headers/database.h"
#include "../../headers/hashtable.h"
#include "../../headers/btree.h"
#include "../../headers/utils.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

struct KVPair *set_data(struct KVPair *data, const string_t key, value_t value, const enum TellyTypes type) {
  if (data) {
    if (data->type == TELLY_STR && type != TELLY_STR) {
      free(data->value->string.value);
    } else if (data->type == TELLY_HASHTABLE && type != TELLY_HASHTABLE) {
      free_hashtable(data->value->hashtable);
    } else if (data->type == TELLY_LIST && type != TELLY_LIST) {
      free_list(data->value->list);
    }

    switch (data->type = type) {
      case TELLY_STR: {
        const uint32_t size = value.string.len + 1;
        data->value->string.len = value.string.len;

        if (data->value->string.value) {
          data->value->string.value = realloc(data->value->string.value, size);
        } else {
          data->value->string.value = malloc(size);
        }

        memcpy(data->value->string.value, value.string.value, size);
        break;
      }

      case TELLY_NUM:
        data->value->number = value.number;
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

      default:
        break;
    }

    return data;
  } else {
    struct BTree *cache = get_cache();
    return insert_kv_to_btree(cache, key, &value, type, -1, -1);
  }
}
