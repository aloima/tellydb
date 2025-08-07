#include <telly.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static void run(struct CommandEntry entry) {
  if (entry.data->arg_count != 2) {
    if (entry.client) WRONG_ARGUMENT_ERROR(entry.client, "RENAME", 6);
    return;
  }

  const string_t search = entry.data->args[0];
  const uint64_t index = hash(search.value, search.len);

  struct BTreeValue *value = find_value_from_btree(entry.database->cache, index, (char *) search.value, (bool (*)(void *, void *)) check_correct_kv);

  if (value) {
    struct KVPair *kv = value->data;

    if ((search.len == kv->key.len) && (strncmp(search.value, kv->key.value, search.len) == 0)) {
      string_t *old = &kv->key;
      const string_t new = entry.data->args[1];

      if (get_data(entry.database, new)) {
        _write(entry.client, "-The new key already exists\r\n", 26);
        return;
      }

      value->index = hash(new.value, new.len);
      old->len = new.len;
      old->value = realloc(old->value, new.len);
      memcpy(old->value, new.value, new.len);

      if (entry.client) WRITE_OK(entry.client);
      return;
    }
  }

  if (entry.client) WRITE_NULL_REPLY(entry.client);
}

const struct Command cmd_rename = {
  .name = "RENAME",
  .summary = "Renames existing key to new key.",
  .since = "0.1.7",
  .complexity = "O(1)",
  .permissions = P_WRITE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
