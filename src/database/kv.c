#include "../../headers/telly.h"

#include <stdlib.h>

void set_kv(struct KVPair *pair, char *key, void *value, enum TellyTypes type) {
  pair->type = type;
  set_string(&pair->key, key, -1, true);

  switch (type) {
    case TELLY_STR:
      set_string(&pair->value.string, value, -1, true);
      break;

    case TELLY_INT:
      pair->value.integer = *((int *) value);
      break;

    case TELLY_BOOL:
      pair->value.boolean = *((bool *) value);
      break;

    case TELLY_HASHTABLE:
      pair->value.hashtable = value;
      break;

    case TELLY_LIST:
      pair->value.list = value;
      break;

    case TELLY_NULL:
      pair->value.null = NULL;
      break;
  }
}

void *get_kv_val(struct KVPair *pair, enum TellyTypes type) {
  switch (type) {
    case TELLY_STR:
      return pair->value.string.value;

    case TELLY_INT:
      return &pair->value.integer;

    case TELLY_BOOL:
      return &pair->value.boolean;

    case TELLY_HASHTABLE:
      return pair->value.hashtable;

    case TELLY_LIST:
      return pair->value.list;

    case TELLY_NULL:
      return NULL;

    default:
      return NULL;
  }
}

void free_kv(struct KVPair *pair) {
  switch (pair->type) {
    case TELLY_STR:
      free(pair->value.string.value);
      break;

    case TELLY_HASHTABLE:
      free_hashtable(pair->value.hashtable);
      break;

    case TELLY_LIST:
      free_list(pair->value.list);
      break;

    default:
      break;
  }

  free(pair->key.value);
  free(pair);
}
