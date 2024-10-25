#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/utils.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

static void lpush_to_list(struct List *list, void *value, enum TellyTypes type) {
  struct ListNode *node = create_listnode(value, type);
  node->next = list->begin;
  list->begin = node;
  list->size += 1;

  if (list->size == 1) {
    list->end = node;
  } else {
    node->next->prev = node;
  }
}

static void run(struct Client *client, respdata_t *data) {
  if (data->count < 3) {
    if (client) WRONG_ARGUMENT_ERROR(client, "LPUSH", 5);
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

    list = kv->value;
  } else {
    list = create_list();
    set_data(kv, key, list, TELLY_LIST);
  }

  const uint32_t value_count = data->count - 2;

  for (uint32_t i = 0; i < value_count; ++i) {
    string_t input = data->value.array[2 + i]->value.string;
    char *input_value = input.value;
    bool is_true = streq(input_value, "true");

    if (is_integer(input_value)) {
      const long number = atol(input_value);
      long *value = malloc(sizeof(long));
      memcpy(value, &number, sizeof(long));

      lpush_to_list(list, value, TELLY_NUM);
    } else if (is_true || streq(input_value, "false")) {
      bool *value = malloc(sizeof(bool));
      memset(value, is_true, sizeof(bool));

      lpush_to_list(list, value, TELLY_BOOL);
    } else if (streq(input_value, "null")) {
      lpush_to_list(list, NULL, TELLY_NULL);
    } else {
      string_t *value = malloc(sizeof(string_t));
      const uint32_t size = input.len + 1;
      value->len = input.len;
      value->value = malloc(size);
      memcpy(value->value, input_value, size);

      lpush_to_list(list, value, TELLY_STR);
    }
  }

  if (client) {
    char buf[14];
    const size_t nbytes = sprintf(buf, ":%d\r\n", value_count);
    _write(client, buf, nbytes);
  }
}

struct Command cmd_lpush = {
  .name = "LPUSH",
  .summary = "Pushes element(s) to beginning of the list and returns pushed element count.",
  .since = "0.1.3",
  .complexity = "O(N) where N is written element count",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
