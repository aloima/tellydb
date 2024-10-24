#include "../../headers/database.h"
#include "../../headers/hashtable.h"
#include "../../headers/utils.h"

#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>

void set_kv(struct KVPair *kv, const string_t key, void *value, const enum TellyTypes type, const off_t start_at, const off_t end_at) {
  kv->pos.start_at = start_at;
  kv->pos.end_at = end_at;

  const uint32_t key_size = key.len + 1;

  kv->type = type;
  kv->value = value;
  kv->key.len = key.len;
  kv->key.value = malloc(key_size);
  memcpy(kv->key.value, key.value, key_size);
}

void free_kv(struct KVPair *kv) {
  if (kv->value) {
    switch (kv->type) {
      case TELLY_STR:
        free(((string_t *) kv->value)->value);
        free(kv->value);
        break;

      case TELLY_HASHTABLE:
        free_hashtable(kv->value);
        break;

      case TELLY_LIST:
        free_list(kv->value);
        break;

      case TELLY_NULL:
        break;

      default:
        free(kv->value);
        break;
    }
  }

  free(kv->key.value);
  free(kv);
}
