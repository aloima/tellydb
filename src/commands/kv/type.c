#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stddef.h>

static void run(struct Client *client, commanddata_t *command) {
  if (client) {
    if (command->arg_count != 1) {
      WRONG_ARGUMENT_ERROR(client, "TYPE", 4);
      return;
    }

    const char *key = command->args[0].value;
    struct KVPair *res = get_data(key);

    if (res) {
      switch (res->type) {
        case TELLY_NULL:
          _write(client, "+null\r\n", 7);
          break;

        case TELLY_NUM:
          _write(client, "+number\r\n", 9);
          break;

        case TELLY_STR:
          _write(client, "+string\r\n", 9);
          break;

        case TELLY_HASHTABLE:
          _write(client, "+hash table\r\n", 13);
          break;

        case TELLY_LIST:
          _write(client, "+list\r\n", 7);
          break;

        case TELLY_BOOL:
          _write(client, "+boolean\r\n", 10);
          break;

        default:
          break;
      }
    } else WRITE_NULL_REPLY(client);
  }
}

struct Command cmd_type = {
  .name = "TYPE",
  .summary = "Returns type of the value.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
