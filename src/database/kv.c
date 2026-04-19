#include <telly.h>

void set_kv(struct KVPair *kv, const string_t key, void *value, const enum TellyTypes type, const uint64_t *expire_at_p) {
  kv->key.value = malloc(key.len);
  if (!kv->key.value) return;

  kv->hashed = OPENSSL_LH_strhash(key.value);
  kv->key.len = key.len;
  memcpy(kv->key.value, key.value, key.len);

  kv->type = type;
  kv->value = value;

  if (expire_at_p != NULL) {
    kv->expire.enabled = true;
    kv->expire.at = *expire_at_p;
  } else {
    kv->expire.enabled = false;
  }
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
