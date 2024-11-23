#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/hashtable.h"
#include "../../../headers/utils.h"

#include <stddef.h>

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (client) {
    if (command->arg_count != 2) {
      WRONG_ARGUMENT_ERROR(client, "HTYPE", 5);
      return;
    }

    if (password->permissions & P_READ) {
      const char *key = command->args[0].value;
      const struct KVPair *kv = get_data(key);

      if (kv) {
        if (kv->type == TELLY_HASHTABLE) {
          char *name = command->args[1].value;
          struct FVPair *fv = get_fv_from_hashtable(kv->value, name);

          switch (fv->type) {
            case TELLY_NULL:
              _write(client, "+null\r\n", 7);
              break;

            case TELLY_NUM:
              _write(client, "+number\r\n", 9);
              break;

            case TELLY_STR:
              _write(client, "+string\r\n", 9);
              break;

            case TELLY_BOOL:
              _write(client, "+boolean\r\n", 10);
              break;

            default:
              break;
          }
        } else _write(client, "-Invalid type for 'HTYPE' command\r\n", 35);
      } else WRITE_NULL_REPLY(client);
    } else {
      _write(client, "-Not allowed to use this command, need P_READ\r\n", 47);
    }
  }
}

const struct Command cmd_htype = {
  .name = "HTYPE",
  .summary = "Returns type of the field from hash table.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
