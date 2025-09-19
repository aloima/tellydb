#include <telly.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static string_t run(struct CommandEntry entry) {
  if (entry.data->arg_count != 2) {
    PASS_NO_CLIENT(entry.client);
    return WRONG_ARGUMENT_ERROR("RENAME");
  }

  const string_t search = entry.data->args[0];
  const uint64_t index = hash(search.value, search.len);

  // TODO: implement
  struct KVPair *kv = NULL;

  if (kv) {
    if ((search.len == kv->key.len) && (strncmp(search.value, kv->key.value, search.len) == 0)) {
      string_t *old = &kv->key;
      const string_t new = entry.data->args[1];

      if (get_data(entry.database, new)) {
        PASS_NO_CLIENT(entry.client);
        return RESP_ERROR_MESSAGE("The new key already exists");
      }

      // value->index = hash(new.value, new.len);
      old->len = new.len;
      old->value = realloc(old->value, new.len);
      memcpy(old->value, new.value, new.len);

      PASS_NO_CLIENT(entry.client);
      return RESP_OK();
    }
  }

  PASS_NO_CLIENT(entry.client);
  return RESP_NULL(entry.client->protover);
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
