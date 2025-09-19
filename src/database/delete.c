#include <telly.h>

#include <stddef.h>
#include <stdbool.h>

bool delete_data(struct Database *database, const string_t key) {
  const uint64_t index = hash((char *) key.value, key.len);
  // TODO: implement
  return true;
}
