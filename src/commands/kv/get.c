#include "../../../headers/telly.h"

#include <stddef.h>

static void run(struct CommandEntry entry) {
  if (entry.client) {
    if (entry.data->arg_count != 1) {
      WRONG_ARGUMENT_ERROR(entry.client, "GET", 3);
      return;
    }

    if (entry.password->permissions & P_READ) {
      const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

      if (kv) {
        write_value(entry.client, kv->value, kv->type);
      } else WRITE_NULL_REPLY(entry.client);
    } else {
      _write(entry.client, "-Not allowed to use this command, need P_READ\r\n", 47);
    }
  }
}

const struct Command cmd_get = {
  .name = "GET",
  .summary = "Gets value.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
