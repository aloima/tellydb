#include "../../headers/database.h"
#include "../../headers/hashtable.h"
#include "../../headers/utils.h"

#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>

void set_kv(struct KVPair *kv, const string_t key, void *value, const enum TellyTypes type) {
  kv->type = type;
  kv->value = value;
  kv->key.len = key.len;
  kv->key.value = malloc(key.len);
  memcpy(kv->key.value, key.value, key.len);
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

bool check_correct_kv(struct KVPair *kv, string_t *key) {
  return ((key->len == kv->key.len) && (memcmp(kv->key.value, key->value, key->len) == 0));
}
