#include <telly.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

static void inline lpush_to_list(struct List *list, void *value, enum TellyTypes type) {
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

static string_t run(struct CommandEntry entry) {
  if (entry.data->arg_count < 2) {
    PASS_NO_CLIENT(entry.client);
    return WRONG_ARGUMENT_ERROR("LPUSH");
  }

  const string_t key = entry.data->args[0];
  struct KVPair *kv = get_data(entry.database, key);
  struct List *list;

  if (kv) {
    if (kv->type != TELLY_LIST) {
      PASS_NO_CLIENT(entry.client);
      return INVALID_TYPE_ERROR("LPUSH");
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

      lpush_to_list(list, value, TELLY_NUM);
    } else if (is_true || streq(input_value, "false")) {
      bool *value = malloc(sizeof(bool));
      *value = is_true;

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

  PASS_NO_CLIENT(entry.client);
  const size_t nbytes = sprintf(entry.buffer, ":%" PRIu32 "\r\n", entry.data->arg_count - 1);
  return CREATE_STRING(entry.buffer, nbytes);
}

const struct Command cmd_lpush = {
  .name = "LPUSH",
  .summary = "Pushes element(s) to beginning of the list.",
  .since = "0.1.3",
  .complexity = "O(N) where N is written element count",
  .permissions = P_WRITE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
