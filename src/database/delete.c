#include <telly.h>

#include <stddef.h>

bool delete_data(struct Database *database, const string_t key) {
  const uint64_t index = hash((char *) key.value, key.len);
  return delete_value_from_btree(database->cache, index, (void (*)(void *)) free_kv);
}
