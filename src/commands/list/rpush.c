#include "../../../headers/telly.h"

#include <stdio.h>
#include <string.h>
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

static void run(struct CommandEntry entry) {
  if (entry.data->arg_count < 2) {
    if (entry.client) WRONG_ARGUMENT_ERROR(entry.client, "RPUSH", 5);
    return;
  }

  if (entry.password->permissions & (P_READ | P_WRITE)) {
    const string_t key = entry.data->args[0];
    struct KVPair *kv = get_data(entry.database, key);
    struct List *list;

    if (kv) {
      if (kv->type != TELLY_LIST) {
        if (entry.client) _write(entry.client, "-Not allowed to use this command, need P_READ\r\n", 47);
        return;
      }

      list = kv->value;
    } else {
      list = create_list();
      set_data(entry.database, kv, key, list, TELLY_LIST);
    }

    for (uint32_t i = 1; i < entry.data->arg_count; ++i) {
      string_t input = entry.data->args[i];
      char *input_value = input.value;
      bool is_true = streq(input_value, "true");

      if (is_integer(input_value)) {
        const long number = atol(input_value);
        long *value = malloc(sizeof(long));
        *value = number;

        rpush_to_list(list, value, TELLY_NUM);
      } else if (is_true || streq(input_value, "false")) {
        bool *value = malloc(sizeof(bool));
        *value = is_true;

        rpush_to_list(list, value, TELLY_BOOL);
      } else if (streq(input_value, "null")) {
        rpush_to_list(list, NULL, TELLY_NULL);
      } else {
        string_t *value = malloc(sizeof(string_t));
        const uint32_t size = input.len + 1;
        value->len = input.len;
        value->value = malloc(size);
        memcpy(value->value, input_value, size);

        rpush_to_list(list, value, TELLY_STR);
      }
    }

    if (entry.client) {
      char buf[14];
      const size_t nbytes = sprintf(buf, ":%d\r\n", entry.data->arg_count - 1);
      _write(entry.client, buf, nbytes);
    }
  } else if (entry.client) {
    _write(entry.client, "-Not allowed to use this command, need P_WRITE\r\n", 48);
  }
}

const struct Command cmd_rpush = {
  .name = "RPUSH",
  .summary = "Pushes element(s) to ending of the list.",
  .since = "0.1.3",
  .complexity = "O(N) where N is written element count",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
