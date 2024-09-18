#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (client && data->count < 3) {
    write(client->connfd, "-Wrong argument count for 'LPUSH' command\r\n", 43);
    return;
  }

  string_t key = data->value.array[1]->value.string;
  struct KVPair *pair = get_data(key.value, conf);
  struct List *list;

  if (pair) {
    if (client && pair->type != TELLY_LIST) {
      write(client->connfd, "-Value stored by the key is not a list\r\n", 40);
      return;
    } else {
      list = pair->value.list;
    }
  } else {
    list = create_list();

    pair = set_data((struct KVPair) {
      .key = key,
      .value = {
        .list = list
      },
      .type = TELLY_LIST
    }, conf);
  }

  const uint32_t value_count = data->count - 2;

  for (uint32_t i = 0; i < value_count; ++i) {
    char *value = data->value.array[2 + i]->value.string.value;
    bool is_true = streq(value, "true");

    if (is_integer(value)) {
      int value_as_int = atoi(value);
      lpush_to_list(list, &value_as_int, TELLY_INT);
    } else if (is_true || streq(value, "false")) {
      lpush_to_list(list, &is_true, TELLY_BOOL);
    } else if (streq(value, "null")) {
      lpush_to_list(list, NULL, TELLY_NULL);
    } else {
      lpush_to_list(list, value, TELLY_STR);
    }
  }

  if (client) {
    const uint32_t buf_len = get_digit_count(value_count) + 3;
    char buf[buf_len + 1];
    sprintf(buf, ":%d\r\n", value_count);

    write(client->connfd, buf, buf_len);
  }
}

struct Command cmd_lpush = {
  .name = "LPUSH",
  .summary = "Pushes element(s) to beginning of the list and returns pushed element count.",
  .since = "1.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
