#include "../../headers/telly.h"

#include <stdbool.h>

bool delete_data(const string_t key) {
  return delete_kv_from_cache(key.value, key.len);
}
