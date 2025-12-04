#include <telly.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static inline uint64_t add_to_index(const uint64_t index, const uint64_t capacity) {
  return ((index + 1) % capacity);
}

static inline struct KVPair *search_kv(struct Database *database, const string_t search, const uint64_t capacity, uint64_t *at) {
  const uint64_t start_idx = (*at = (hash(search.value, search.len) % capacity));
  struct KVPair *kv;

  while (true) {
    kv = database->data[*at];
    *at = add_to_index(*at, capacity);

    if (!kv || (*at == start_idx)) {
      return NULL;
    }

    if ((search.len == kv->key.len) && (memcmp(search.value, kv->key.value, search.len) == 0)) {
      break;
    }
  }

  return kv;
}

static inline void place_kv(struct Database *database, struct KVPair *kv, const uint64_t capacity) {
  uint64_t index = (kv->hashed % capacity);

  while (database->data[index]) {
    index = add_to_index(index, capacity);
  }

  database->data[index] = kv;
}

static inline void change_kv(struct KVPair *kv, const string_t name) {
  string_t *area = &kv->key;
  area->len = name.len;
  area->value = realloc(area->value, name.len);
  memcpy(area->value, name.value, name.len);

  kv->hashed = hash(name.value, name.len);
}

static inline void shift_others(struct Database *database, struct KVPair *kv, const uint64_t at, const uint64_t capacity) {
  uint64_t prev = at;
  uint64_t next = add_to_index(prev, capacity);

  while (database->data[next]) {
    if ((database->data[next]->hashed % capacity) == (kv->hashed % capacity)) {
      database->data[prev] = database->data[next];
      prev = add_to_index(prev, capacity);
      next = add_to_index(next, capacity);
    } else {
      database->data[prev] = NULL;
      break;
    }
  }

  database->data[prev] = NULL;
}

static string_t run(struct CommandEntry *entry) {
  if (entry->data->arg_count != 2) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("RENAME");
  }

  struct KVPair **data = entry->database->data;
  const uint64_t capacity = entry->database->size.capacity;

  const string_t search = entry->data->args[0];
  const string_t name = entry->data->args[1];

  if (get_data(entry->database, name)) {
    PASS_NO_CLIENT(entry->client);
    return RESP_ERROR_MESSAGE("The new key already exists");
  }

  uint64_t at;
  struct KVPair *kv = search_kv(entry->database, search, capacity, &at);

  if (!kv) {
    PASS_NO_CLIENT(entry->client);
    return RESP_NULL(entry->client->protover);
  }

  shift_others(entry->database, kv, at, capacity);
  change_kv(kv, name);
  place_kv(entry->database, kv, capacity);

  PASS_NO_CLIENT(entry->client);
  return RESP_OK();
}

const struct Command cmd_rename = {
  .name = "RENAME",
  .summary = "Renames existing key to new key.",
  .since = "0.1.7",
  .complexity = "O(1)",
  .permissions = P_WRITE,
  .flags = CMD_FLAG_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
