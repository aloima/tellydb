#include "../../../headers/telly.h"

#include <stddef.h>

static void run(struct CommandEntry entry) {
  if (entry.client) {
    if (entry.data->arg_count != 2) {
      WRONG_ARGUMENT_ERROR(entry.client, "HTYPE", 5);
      return;
    }

    if (entry.password->permissions & P_READ) {
      const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

      if (kv) {
        if (kv->type == TELLY_HASHTABLE) {
          struct FVPair *fv = get_fv_from_hashtable(kv->value, entry.data->args[1]);

          switch (fv->type) {
            case TELLY_NULL:
              _write(entry.client, "+null\r\n", 7);
              break;

            case TELLY_NUM:
              _write(entry.client, "+number\r\n", 9);
              break;

            case TELLY_STR:
              _write(entry.client, "+string\r\n", 9);
              break;

            case TELLY_BOOL:
              _write(entry.client, "+boolean\r\n", 10);
              break;

            default:
              break;
          }
        } else _write(entry.client, "-Invalid type for 'HTYPE' command\r\n", 35);
      } else WRITE_NULL_REPLY(entry.client);
    } else {
      _write(entry.client, "-Not allowed to use this command, need P_READ\r\n", 47);
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
