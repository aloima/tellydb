#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/hashtable.h"
#include "../../../headers/utils.h"

#include <stdio.h>
#include <stddef.h>

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (client) {
    if (command->arg_count != 1) {
      WRONG_ARGUMENT_ERROR(client, "HLEN", 4);
      return;
    }

    if (password->permissions & P_READ) {
      const struct KVPair *kv = get_data(command->args[0].value);

      if (kv) {
        if (kv->type == TELLY_HASHTABLE) {
          const struct HashTable *table = kv->value;

          char buf[90];
          const size_t nbytes = sprintf(buf, (
            "*3\r\n"
              "+Allocated: %d\r\n"
              "+Filled: %d\r\n"
              "+All (includes next count): %d\r\n"
          ), table->size.allocated, table->size.filled, table->size.all);

          _write(client, buf, nbytes);
        } else {
          _write(client, "-Invalid type for 'HLEN' command\r\n", 34);
        }
      } else WRITE_NULL_REPLY(client);
    } else {
      _write(client, "-Not allowed to use this command, need P_READ\r\n", 47);
    }
  }
}

struct Command cmd_hlen = {
  .name = "HLEN",
  .summary = "Returns field count information of the hash table.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
