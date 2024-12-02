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

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (command->arg_count < 2) {
    if (client) WRONG_ARGUMENT_ERROR(client, "RPUSH", 5);
    return;
  }

  if (password->permissions & (P_READ | P_WRITE)) {
    const string_t key = command->args[0];
    struct KVPair *kv = get_data(key);
    struct List *list;

    if (kv) {
      if (kv->type != TELLY_LIST) {
        if (client) _write(client, "-Not allowed to use this command, need P_READ\r\n", 47);
        return;
      }

      list = kv->value;
    } else {
      list = create_list();
      set_data(kv, key, list, TELLY_LIST);
    }

    for (uint32_t i = 1; i < command->arg_count; ++i) {
      string_t input = command->args[i];
      char *input_value = input.value;
      bool is_true = streq(input_value, "true");

      if (is_integer(input_value)) {
        const long number = atol(input_value);
        long *value = malloc(sizeof(long));
        memcpy(value, &number, sizeof(long));

        rpush_to_list(list, value, TELLY_NUM);
      } else if (is_true || streq(input_value, "false")) {
        bool *value = malloc(sizeof(bool));
        memset(value, is_true, sizeof(bool));

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

    if (client) {
      char buf[14];
      const size_t nbytes = sprintf(buf, ":%d\r\n", command->arg_count - 1);
      _write(client, buf, nbytes);
    }
  } else if (client) {
    _write(client, "-Not allowed to use this command, need P_WRITE\r\n", 48);
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
