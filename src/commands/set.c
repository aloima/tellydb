#include "../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, __attribute__((unused)) struct Configuration *conf) {
  if (data->count < 3) {
    if (client) write(client->connfd, "-missing arguments\r\n", 20);
  } else {
    char *value = data->value.array[2]->value.string.value;

    struct KVPair res;
    res.key = data->value.array[1]->value.string;

    const bool is_true = streq(value, "true");

    if (is_integer(value)) {
      res.type = TELLY_INT;
      res.value.integer = atoi(value);
    } else if (is_true || streq(value, "false")) {
      res.type = TELLY_BOOL;
      res.value.boolean = is_true;
    } else if (streq(value, "null")) {
      res.type = TELLY_NULL;
      res.value.null = NULL;
    } else {
      res.type = TELLY_STR;
      res.value.string = data->value.array[2]->value.string;
    }

    set_data(res, conf);
    if (client) write(client->connfd, "+OK\r\n", 5);
  }
}

struct Command cmd_set = {
  .name = "SET",
  .summary = "Sets value to specified key. If the key already has a value, it is overwritten.",
  .since = "1.0.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
