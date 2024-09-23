#include "../../headers/telly.h"

#include <stdlib.h>

void set_kv(struct KVPair *kv, char *key, void *value, enum TellyTypes type) {
  kv->type = type;
  set_string(&kv->key, key, -1, true);

  switch (type) {
    case TELLY_STR:
      set_string(&kv->value.string, value, -1, true);
      break;

    case TELLY_INT:
      kv->value.integer = *((int *) value);
      break;

    case TELLY_BOOL:
      kv->value.boolean = *((bool *) value);
      break;

    case TELLY_HASHTABLE:
      kv->value.hashtable = value;
      break;

    case TELLY_LIST:
      kv->value.list = value;
      break;

    case TELLY_NULL:
      kv->value.null = NULL;
      break;
  }
}

void *get_kv_val(struct KVPair *kv, enum TellyTypes type) {
  switch (type) {
    case TELLY_STR:
      return kv->value.string.value;

    case TELLY_INT:
      return &kv->value.integer;

    case TELLY_BOOL:
      return &kv->value.boolean;

    case TELLY_HASHTABLE:
      return kv->value.hashtable;

    case TELLY_LIST:
      return kv->value.list;

    case TELLY_NULL:
      return NULL;

    default:
      return NULL;
  }
}

void free_kv(struct KVPair *kv) {
  switch (kv->type) {
    case TELLY_STR:
      free(kv->value.string.value);
      break;

    case TELLY_HASHTABLE:
      free_hashtable(kv->value.hashtable);
      break;

    case TELLY_LIST:
      free_list(kv->value.list);
      break;

    default:
      break;
  }

  free(kv->key.value);
  free(kv);
}
