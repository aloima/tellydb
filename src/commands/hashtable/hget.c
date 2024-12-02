#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/hashtable.h"
#include "../../../headers/utils.h"

#include <stddef.h>

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (client) {
    if (command->arg_count != 2) {
      WRONG_ARGUMENT_ERROR(client, "HGET", 4);
      return;
    }

    if (password->permissions & P_READ) {
      const struct KVPair *kv = get_data(command->args[0]);

      if (kv) {
        if (kv->type == TELLY_HASHTABLE) {
          const struct FVPair *field = get_fv_from_hashtable(kv->value, command->args[1]);

          if (field) {
            write_value(client, field->value, field->type);
          } else WRITE_NULL_REPLY(client);
        } else {
          _write(client, "-Invalid type for 'HGET' command\r\n", 34);
        }
      } else WRITE_NULL_REPLY(client);
    } else {
      _write(client, "-Not allowed to use this command, need P_READ\r\n", 47);
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
