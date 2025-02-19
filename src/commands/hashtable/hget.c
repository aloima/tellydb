#include "../../../headers/telly.h"

#include <stddef.h>

static void run(struct CommandEntry entry) {
  if (entry.client) {
    if (entry.data->arg_count != 2) {
      WRONG_ARGUMENT_ERROR(entry.client, "HGET", 4);
      return;
    }

    if (entry.password->permissions & P_READ) {
      const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

      if (kv) {
        if (kv->type == TELLY_HASHTABLE) {
          const struct FVPair *field = get_fv_from_hashtable(kv->value, entry.data->args[1]);

          if (field) {
            write_value(entry.client, field->value, field->type);
          } else WRITE_NULL_REPLY(entry.client);
        } else {
          _write(entry.client, "-Invalid type for 'HGET' command\r\n", 34);
        }
      } else WRITE_NULL_REPLY(entry.client);
    } else {
      _write(entry.client, "-Not allowed to use this command, need P_READ\r\n", 47);
    }
  }
}

const struct Command cmd_hget = {
  .name = "HGET",
  .summary = "Gets a field from the hash table.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
