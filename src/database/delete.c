#include <telly.h>

bool delete_data(Database *database, string_t key) {
  KeyValue *kv = get_data(database, key);
  if (kv == NULL)
    return false;

  // When deleted, value is not appeared, it is reachable
  const bool deleted = delete_from_hashtable(database->data, &key);
  if (!deleted)
    return false;

  free(kv->key.value);
  free_value(kv->value);

  free(kv);
  return true;
}

void clear_database(Database *database) {
  clear_hashtable(database->data, free_hashtablekeyvalue);
}
