#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/utils.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

static void rpush_to_list(struct List *list, void *value, enum TellyTypes type) {
  struct ListNode *node = create_listnode(value, type);
  node->prev = list->end;
  list->end = node;
  list->size += 1;

  if (list->size == 1) {
    list->begin = node;
  } else {
    node->prev->next = node;
  }
}

static void run(struct Client *client, respdata_t *data) {
  if (data->count < 3) {
    if (client) WRONG_ARGUMENT_ERROR(client, "RPUSH", 5);
    return;
  }

  const string_t key = data->value.array[1]->value.string;
  struct KVPair *kv = get_data(key.value);
  struct List *list;

  if (kv) {
    if (kv->type != TELLY_LIST) {
      if (client) _write(client, "-Value stored at the key is not a list\r\n", 40);
      return;
    }

    list = kv->value->list;
  } else {
    list = create_list();
    set_data(kv, key, (value_t) {
      .list = list
    }, TELLY_LIST);
  }

  const uint32_t value_count = data->count - 2;

  for (uint32_t i = 0; i < value_count; ++i) {
    char *value = data->value.array[2 + i]->value.string.value;
    bool is_true = streq(value, "true");

    if (is_integer(value)) {
      long value_as_long = atol(value);
      rpush_to_list(list, &value_as_long, TELLY_NUM);
    } else if (is_true || streq(value, "false")) {
      rpush_to_list(list, &is_true, TELLY_BOOL);
    } else if (streq(value, "null")) {
      rpush_to_list(list, NULL, TELLY_NULL);
    } else {
      rpush_to_list(list, value, TELLY_STR);
    }
  }

  if (client) {
    char buf[14];
    const size_t nbytes = sprintf(buf, ":%d\r\n", value_count);
    _write(client, buf, nbytes);
  }
}

struct Command cmd_rpush = {
  .name = "RPUSH",
  .summary = "Pushes element(s) to ending of the list and returns pushed element count.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
