#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stddef.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count != 2) {
      WRONG_ARGUMENT_ERROR(client, "GET", 3);
      return;
    }

    const char *key = data->value.array[1]->value.string.value;
    const struct KVPair *kv = get_data(key);

    if (kv) {
      write_value(client, kv->value, kv->type);
    } else WRITE_NULL_REPLY(client);
  }
}

struct Command cmd_get = {
  .name = "GET",
  .summary = "Gets value from specified key.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
