#include <telly.h>

KeyValue *set_data(Database *database, KeyValue *kv, string_t key, void *data, const enum TellyTypes type, const uint64_t *expire_at) {
  if (kv != NULL) {
    free_value(kv->value);
    kv->value.type = type;
    kv->value.data = data;

    if (expire_at != NULL) {
      kv->expiry.enabled = true;
      kv->expiry.at = *expire_at;
    } else {
      kv->expiry.enabled = false;
    }

    return kv;
  }

  kv = malloc(sizeof(KeyValue));
  if (kv == NULL)
    return NULL;

  const int set_ret = set_kv(kv, key, data, type, expire_at);
  if (set_ret == -1) {
    free(kv);
    return NULL;
  }

  HashTableKeyValue *element = (HashTableKeyValue *) insert_into_hashtable(database->data, &kv->key, kv);
  if (element == NULL)
    return NULL;

  return element->value;
}
