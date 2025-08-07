#include <telly.h>

#include <stdio.h>
#include <stdint.h>

static void run(struct CommandEntry entry) {
  if (entry.data->arg_count < 2) {
    if (entry.client) {
      WRONG_ARGUMENT_ERROR(entry.client, "HDEL");
    }

    return;
  }

  const string_t key = entry.data->args[0];
  struct KVPair *kv = get_data(entry.database, key);
  struct HashTable *table;

  if (kv) {
    if (kv->type == TELLY_HASHTABLE) {
      table = kv->value;
    } else {
      if (entry.client) {
        INVALID_TYPE_ERROR(entry.client, "HDEL");
      }

      return;
    }
  } else {
    if (entry.client) _write(entry.client, ":0\r\n", 4);
    return;
  }

  if (entry.client) {
    if (!(entry.password->permissions & P_READ)) {
      WRITE_ERROR_MESSAGE(entry.client, "Not allowed to use this command, need P_READ");
      return;
    }

    const uint32_t old_size = table->size.all;

    for (uint32_t i = 1; i < entry.data->arg_count; ++i) {
      del_field_to_hashtable(table, entry.data->args[i]);
    }

    if (table->size.all == 0) delete_data(entry.database, key);

    char buf[14];
    const size_t nbytes = sprintf(buf, ":%u\r\n", old_size - table->size.all);
    _write(entry.client, buf, nbytes);
  } else {
    for (uint32_t i = 1; i < entry.data->arg_count; ++i) {
      del_field_to_hashtable(table, entry.data->args[i]);
    }

    if (table->size.all == 0) {
      delete_data(entry.database, key);
    }
  }
}

const struct Command cmd_hdel = {
  .name = "HDEL",
  .summary = "Deletes field(s) of the hash table.",
  .since = "0.1.5",
  .complexity = "O(N) where N is written field name count",
  .permissions = P_WRITE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
