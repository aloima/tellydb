#include "../../headers/telly.h"

#include <stddef.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (client) {
    switch (data->count) {
      case 1:
        write(client->connfd, "-missing argument\r\n", 19);
        break;

      case 2: {
        char *key = data->value.array[1]->value.string.value;
        struct KVPair *result = get_data(key, conf);

        if (result) {
          write_value(client->connfd, result->value, result->type);
        } else {
          write(client->connfd, "$-1\r\n", 5);
        }

        break;
      }

      default:
        write(client->connfd, "-additional argument(s)\r\n", 25);
        break;
    }
  }
}

struct Command cmd_get = {
  .name = "GET",
  .summary = "Gets value from specified key.",
  .since = "1.0.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
