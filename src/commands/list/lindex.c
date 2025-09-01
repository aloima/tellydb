#include <telly.h>

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

static void run(struct CommandEntry entry) {
  if (!entry.client) return;
  if (entry.data->arg_count != 2) {
    WRONG_ARGUMENT_ERROR(entry.client, "LINDEX");
    return;
  }

  const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

  if (!kv || kv->type != TELLY_LIST) {
    INVALID_TYPE_ERROR(entry.client, "LINDEX");
    return;
  }

  const char *index_str = entry.data->args[1].value;

  if (!is_integer(index_str)) {
    WRITE_ERROR_MESSAGE(entry.client, "Second argument must be an integer");
    return;
  }

  const struct List *list = kv->value;
  struct ListNode *node;

  if (index_str[0] != '-') {
    const uint64_t index = strtoull(index_str, (char **) NULL, 10);

    if (index == ULLONG_MAX) {
      WRITE_ERROR_MESSAGE(entry.client, "Index exceeded integer bounds");
      return;
    }

    if ((index + 1) > list->size) {
      WRITE_NULL_REPLY(entry.client);
      return;
    }

    node = list->begin;

    for (uint32_t i = 0; i < index; ++i) {
      node = node->next;
    }
  } else {
    const uint64_t index = strtoull(index_str + 1, (char **) NULL, 10);

    if (index == ULLONG_MAX) {
      WRITE_ERROR_MESSAGE(entry.client, "Index exceeded integer bounds");
      return;
    }

    if (index > list->size) {
      WRITE_NULL_REPLY(entry.client);
      return;
    }

    const uint32_t bound = index - 1;
    node = list->end;

    for (uint32_t i = 0; i < bound; ++i) {
      node = node->prev;
    }
  }

  write_value(entry.client, node->value, node->type);
}

const struct Command cmd_lindex = {
  .name = "LINDEX",
  .summary = "Returns element at the index in the list.",
  .since = "0.1.4",
  .complexity = "O(N) where N is index number",
  .permissions = P_READ,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
