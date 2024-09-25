#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, __attribute__((unused)) struct Configuration *conf) {
  if (data->count < 3 && client) {
    WRONG_ARGUMENT_ERROR(client->connfd, "SET", 3);
    return;
  }

  char *value = data->value.array[2]->value.string.value;
  bool get = false;

  for (uint32_t i = 3; i < data->count; ++i) {
    string_t input = data->value.array[i]->value.string;
    char arg[input.len + 1];
    to_uppercase(input.value, arg);

    if (streq(arg, "GET")) get = true;
    else {
      write(client->connfd, "-Invalid argument(s) for 'SET' command\r\n", 40);
      return;
    }
  }

  struct KVPair kv;
  kv.key = data->value.array[1]->value.string;

  const bool is_true = streq(value, "true");

  if (is_integer(value)) {
    kv.type = TELLY_INT;
    kv.value.integer = atoi(value);
  } else if (is_true || streq(value, "false")) {
    kv.type = TELLY_BOOL;
    kv.value.boolean = is_true;
  } else if (streq(value, "null")) {
    kv.type = TELLY_NULL;
    kv.value.null = NULL;
  } else {
    kv.type = TELLY_STR;
    kv.value.string = data->value.array[2]->value.string;
  }

  if (get && client) {
    struct KVPair *val = get_data(data->value.array[1]->value.string.value, conf);
    write_value(client->connfd, val->value, val->type);
    set_data(kv, conf);
  } else if (!get) {
    set_data(kv, conf);
    if (client) write(client->connfd, "+OK\r\n", 5);
  }
}

struct Command cmd_set = {
  .name = "SET",
  .summary = "Sets value to specified key. If the key already has a value, it is overwritten.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
