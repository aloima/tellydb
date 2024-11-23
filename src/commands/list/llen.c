#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/utils.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (client) {
    if (command->arg_count != 1) {
      WRONG_ARGUMENT_ERROR(client, "LLEN", 4);
      return;
    }

    if (password->permissions & P_READ) {
      const char *key = command->args[0].value;
      const struct KVPair *kv = get_data(key);

      if (!kv) {
        _write(client, ":0\r\n", 4);
        return;
      } else if (kv->type != TELLY_LIST) {
        _write(client, "-Value stored at the key is not a list\r\n", 40);
        return;
      }

      char buf[14];
      const size_t nbytes = sprintf(buf, ":%d\r\n", ((struct List *) kv->value)->size);
      _write(client, buf, nbytes);
    } else {
      _write(client, "-Not allowed to use this command, need P_READ\r\n", 47);
    }
  }
}

const struct Command cmd_llen = {
  .name = "LLEN",
  .summary = "Returns length of the list.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
