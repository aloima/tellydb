#include "../../headers/database.h"
#include "../../headers/hashtable.h"
#include "../../headers/utils.h"

#include <stdlib.h>

#include <unistd.h>

void set_kv(struct KVPair *kv, string_t key, value_t *value, enum TellyTypes type, const off_t pos) {
  kv->type = type;
  kv->pos = pos;

  if (!kv->key) {
    kv->key = malloc(sizeof(string_t));
    set_string(kv->key, key.value, key.len, true);
  } else {
    set_string(kv->key, key.value, key.len, false);
  }

  if (!kv->value && type != TELLY_UNSPECIFIED) {
    kv->value = malloc(sizeof(value_t));
  }

  switch (type) {
    case TELLY_STR:
      set_string(&kv->value->string, value->string.value, value->string.len, true);
      break;

    case TELLY_INT:
      kv->value->integer = value->integer;
      break;

    case TELLY_BOOL:
      kv->value->boolean = value->boolean;
      break;

    case TELLY_HASHTABLE:
      kv->value->hashtable = value->hashtable;
      break;

    case TELLY_LIST:
      kv->value->list = value->list;
      break;

    case TELLY_NULL:
      kv->value->null = NULL;
      break;

    case TELLY_UNSPECIFIED:
      break;
  }
}

void free_kv(struct KVPair *kv) {
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
  free(kv->key->value);
  free(kv->key);
  free(kv);
}
