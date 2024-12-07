#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <string.h>
#include <stdint.h>

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (command->arg_count != 2) {
    if (client) WRONG_ARGUMENT_ERROR(client, "RENAME", 6);
    return;
  }

  // TODO: conflict kv names
  if (password->permissions & P_WRITE) {
    const string_t search = command->args[0];
    const uint64_t index = hash(search.value, search.len);

    struct BTreeValue *value = find_value_from_btree(get_cache(), index, (char *) search.value, (bool (*)(void *, void *)) check_correct_kv);

    if (value) {
      struct KVPair *kv = value->data;

      if ((search.len == kv->key.len) && (strncmp(search.value, kv->key.value, search.len) == 0)) {
        string_t *old = &kv->key;
        const string_t new = command->args[1];

        if (get_kv_from_cache(new.value, new.len)) {
          _write(client, "-The new key already exists\r\n", 26);
          return;
        }

        value->index = hash(new.value, new.len);
        old->len = new.len;
        old->value = realloc(old->value, new.len);
        memcpy(old->value, new.value, new.len);

        if (client) WRITE_OK(client);
        return;
      }
    }

    if (client) WRITE_NULL_REPLY(client);
  } else if (client) {
    _write(client, "-Not allowed to use this command, need P_WRITE\r\n", 48);
  }
}

const struct Command cmd_rename = {
  .name = "RENAME",
  .summary = "Renames existing key to new key.",
  .since = "0.1.7",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
