#include "../../../headers/telly.h"

#include <stddef.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (client && data->count != 2) {
    write(client->connfd, "-Wrong argument count for 'LPOP' command\r\n", 42);
    return;
  }

  string_t key = data->value.array[1]->value.string;
  struct KVPair *pair = get_data(key.value, conf);
  struct List *list;

  if (pair) {
    if (client && pair->type != TELLY_LIST) {
      write(client->connfd, "-Value stored by the key is not a list\r\n", 40);
      return;
    }

    list = pair->value.list;

    if (client) write_value(client->connfd, list->begin->value, list->begin->type);
    lpop_to_list(list);
  } else {
    write(client->connfd, "$-1\r\n", 5);
  }
}

struct Command cmd_lpop = {
  .name = "LPOP",
  .summary = "Removes and returns first element(s) of the list stored at the key.",
  .since = "1.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
