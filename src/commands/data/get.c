#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stdio.h>
#include <stddef.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count != 2) {
      WRONG_ARGUMENT_ERROR(client->connfd, "GET", 3);
      return;
    }

    char *key = data->value.array[1]->value.string.value;
    struct KVPair *result = get_data(key);

    if (result) {
      write_value(client->connfd, *result->value, result->type);
    } else {
      write(client->connfd, "$-1\r\n", 5);
    }
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
