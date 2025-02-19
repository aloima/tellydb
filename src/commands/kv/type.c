#include "../../../headers/telly.h"

#include <stddef.h>

static void run(struct CommandEntry entry) {
  if (entry.client) {
    if (entry.data->arg_count != 1) {
      WRONG_ARGUMENT_ERROR(entry.client, "TYPE", 4);
      return;
    }

    struct KVPair *res = get_data(entry.database, entry.data->args[0]);

    if (res) {
      switch (res->type) {
        case TELLY_NULL:
          _write(entry.client, "+null\r\n", 7);
          break;

        case TELLY_NUM:
          _write(entry.client, "+number\r\n", 9);
          break;

        case TELLY_STR:
          _write(entry.client, "+string\r\n", 9);
          break;

        case TELLY_HASHTABLE:
          _write(entry.client, "+hash table\r\n", 13);
          break;

        case TELLY_LIST:
          _write(entry.client, "+list\r\n", 7);
          break;

        case TELLY_BOOL:
          _write(entry.client, "+boolean\r\n", 10);
          break;

        default:
          break;
      }
    } else WRITE_NULL_REPLY(entry.client);
  }
}

const struct Command cmd_type = {
  .name = "TYPE",
  .summary = "Returns type of the value.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
