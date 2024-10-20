#include "../../headers/database.h"
#include "../../headers/hashtable.h"
#include "../../headers/utils.h"

#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>

void set_kv(struct KVPair *kv, const string_t key, value_t *value, const enum TellyTypes type, const off_t start_at, const off_t end_at) {
  kv->pos.start_at = start_at;
  kv->pos.end_at = end_at;

  const uint32_t key_size = key.len + 1;

  kv->type = type;
  kv->key.len = key.len;
  kv->key.value = malloc(key_size);
  memcpy(kv->key.value, key.value, key_size);

  if (value) {
    if (!kv->value) kv->value = malloc(sizeof(value_t));

    switch (type) {
      case TELLY_NULL:
        kv->value->null = NULL;
        break;

      case TELLY_NUM:
        kv->value->number = value->number;
        break;

      case TELLY_STR: {
        const uint32_t size = value->string.len + 1;
        kv->value->string.len = value->string.len;
        kv->value->string.value = malloc(size);
        memcpy(kv->value->string.value, value->string.value, size);
        break;
      }

      case TELLY_BOOL:
        kv->value->boolean = value->boolean;
        break;

      case TELLY_HASHTABLE:
        kv->value->hashtable = value->hashtable;
        break;

      case TELLY_LIST:
        kv->value->list = value->list;
        break;

      default:
        break;
    }
  }
}

void free_kv(struct KVPair *kv) {
  if (kv->value) {
    switch (kv->type) {
      case TELLY_STR:
        free(kv->value->string.value);
        break;

      case TELLY_HASHTABLE:
        free_hashtable(kv->value->hashtable);
        break;

      case TELLY_LIST:
        free_list(kv->value->list);
        break;

      default:
        break;
    }

    free(kv->value);
  }

  free(kv->key.value);
  free(kv);
}
