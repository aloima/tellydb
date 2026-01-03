#include <telly.h>

#include <string.h>
#include <stdlib.h>

#include <gmp.h>

void set_kv(struct KVPair *kv, const string_t key, void *value, const enum TellyTypes type) {
  kv->hashed = hash(key.value, key.len);
  kv->key.len = key.len;
  kv->key.value = malloc(key.len);
  if (!kv->key.value) return;
  memcpy(kv->key.value, key.value, key.len);

  kv->type = type;
  kv->value = value;
}

void free_value(const enum TellyTypes type, void *value) {
  switch (type) {
    case TELLY_NULL:
      break;

    case TELLY_INT:
      mpz_clear(*((mpz_t *) value));
      free(value);
      break;

    case TELLY_DOUBLE:
      mpf_clear(*((mpf_t *) value));
      free(value);
      break;

    case TELLY_STR:
      free(((string_t *) value)->value);
      free(value);
      break;

    case TELLY_BOOL:
      free(value);
      break;

    case TELLY_HASHTABLE:
      free_hashtable(value);
      break;

    case TELLY_LIST:
      free_list(value);
      break;
  }
}

void free_kv(struct KVPair *kv) {
  free_value(kv->type, kv->value);
  free(kv->key.value);
  free(kv);
}
