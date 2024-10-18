#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stddef.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count != 2) {
      WRONG_ARGUMENT_ERROR(client, "TYPE", 4);
      return;
    }

    const char *key = data->value.array[1]->value.string.value;
    struct KVPair *res = get_data(key);

    if (res) {
      switch (res->type) {
        case TELLY_NULL:
          _write(client, "+null\r\n", 7);
          break;

        case TELLY_INT:
          _write(client, "+integer\r\n", 10);
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
    } else {
      _write(client, "$-1\r\n", 5);
    }
  }
}

struct Command cmd_type = {
  .name = "TYPE",
  .summary = "Returns type of value of key.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
