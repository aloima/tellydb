#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (client) {
    if (data->count != 2) {
      WRONG_ARGUMENT_ERROR(client->connfd, "LLEN", 4);
      return;
    }

    string_t key = data->value.array[1]->value.string;
    struct KVPair *pair = get_data(key.value, conf);

    if (!pair) {
      write(client->connfd, ":0\r\n", 4);
      return;
    } else if (pair->type != TELLY_LIST) {
      write(client->connfd, "-Value stored at the key is not a list\r\n", 40);
      return;
    }

    const uint64_t size = pair->value->list->size;
    const uint32_t buf_len = 3 + get_digit_count(pair->value->list->size);
    char buf[buf_len + 1];
    sprintf(buf, ":%ld\r\n", size);

    write(client->connfd, buf, buf_len);
  }
}

struct Command cmd_llen = {
  .name = "LLEN",
  .summary = "Returns length of the list stored at the key.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
