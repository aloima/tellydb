#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (data->count != 2 && client) {
    WRONG_ARGUMENT_ERROR(client->connfd, "INCR", 4);
    return;
  }

  string_t key = data->value.array[1]->value.string;
  struct KVPair *result = get_data(key.value, conf);

  if (!result) {
    set_data(NULL, key, (value_t) {
      .integer = 0
    }, TELLY_INT);
    if (client) write(client->connfd, ":0\r\n", 4);
  } else if (result->type == TELLY_INT) {
    result->value->integer += 1;

    if (client) {
      const uint32_t buf_len = get_digit_count(result->value->integer) + 3;
      char buf[buf_len + 1];

      sprintf(buf, ":%d\r\n", result->value->integer);
      write(client->connfd, buf, buf_len);
    }
  } else if (client) {
    write(client->connfd, "-Invalid type for 'INCR' command\r\n", 34);
  }
}

struct Command cmd_incr = {
  .name = "INCR",
  .summary = "Increments value from specified key.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
