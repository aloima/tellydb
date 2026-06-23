#include <telly.h>

int set_kv(KeyValue *kv, const string_t key, void *value, const enum TellyTypes type, const uint64_t *expire_at) {
  kv->key.value = malloc(key.len + 1);
  if (kv->key.value == NULL)
    return -1;

  kv->key.len = key.len;
  ASSERT(memcpy(kv->key.value, key.value, key.len), !=, NULL);
  kv->key.value[key.len] = '\0';

  kv->value.data = value;
  kv->value.type = type;

  if (expire_at != NULL) {
    kv->expiry.enabled = true;
    kv->expiry.at = *expire_at;
  } else {
    kv->expiry.enabled = false;
  }

  return 0;
}

ExpiryState check_kv_expiry(Database *database, KeyValue *kv) {
  Expiry expiry = kv->expiry;
  if (!expiry.enabled)
    return EXPIRY_NOT_EXPIRED;

  struct timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    return EXPIRY_SYSCALL_ERROR;

  const uint64_t now = (ts.tv_sec * 1e3) + (ts.tv_nsec / 1e6);

  if (expiry.at <= now) {
    if (!delete_from_hashtable(database->data, &kv->key))
      return EXPIRY_DELETING_ERROR;

    return EXPIRY_EXPIRED;
  }

  return EXPIRY_NOT_EXPIRED;
}

void free_list_value(void *data) {
  Value *value = (Value *) data;
  free_value(*value);
  free(value);
}

void free_hashtablekeyvalue(HashTableElement element) {
  const HashTableKeyValue *kv = (HashTableKeyValue *) ((void *) &element);
  KeyValue *value = kv->value;

  free_kv(value);
}

void free_namevalue(void *data) {
  NameValue *element = (NameValue *) data;
  free_value(element->value);
  free(element->name.value);
}

void free_value(Value value) {
  const enum TellyTypes type = value.type;
  void *data = value.data;

  switch (type) {
    case TELLY_NULL: case TELLY_UNKNOWN:
      break;

    case TELLY_INT:
      mpz_clear(*((mpz_t *) data));
      free(data);
      break;

    case TELLY_DOUBLE:
      mpf_clear(*((mpf_t *) data));
      free(data);
      break;

    case TELLY_STR:
      free(((string_t *) data)->value);
      free(data);
      break;

    case TELLY_BOOL:
      free(data);
      break;

    case TELLY_HASHTABLE:
      destroy_hashtable(data, free_hashtablekeyvalue);
      break;

    case TELLY_LIST:
      ll_free(data, free_list_value);
      free(data);
      break;
  }
}

void free_kv(KeyValue *kv) {
  free_value(kv->value);
  free(kv->key.value);
  free(kv);
}
