#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stddef.h>

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (client) {
    if (command->arg_count != 1) {
      WRONG_ARGUMENT_ERROR(client, "GET", 3);
      return;
    }

    if (password->permissions & P_READ) {
      const char *key = command->args[0].value;
      const struct KVPair *kv = get_data(key);

      if (kv) {
        write_value(client, kv->value, kv->type);
      } else WRITE_NULL_REPLY(client);
    } else {
      _write(client, "-Not allowed to use this command, need P_READ\r\n", 47);
    }
  }
}

struct Command cmd_get = {
  .name = "GET",
  .summary = "Gets value.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
