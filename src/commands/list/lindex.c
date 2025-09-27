#include <telly.h>

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

enum SearchDirection {
  FORWARD,
  BACKWARD
};

static inline struct ListNode *get_node(const struct List *list, const uint32_t index, const enum SearchDirection direction) {
  uint32_t effective_index;

  switch (direction) {
    case FORWARD:
      if (index >= list->size) {
        return NULL;
      }

      effective_index = index;
      break;

    case BACKWARD:
      if (index > list->size) {
        return NULL;
      }

      effective_index = list->size - index;
      break;
  }

  struct ListNode *node;

  if (effective_index < (list->size / 2)) {
    node = list->begin;

    for (uint32_t i = 0; i < effective_index; ++i) {
      node = node->next;
    }
  } else {
    const uint32_t bound = list->size - effective_index - 1;
    node = list->end;

    for (uint32_t i = 0; i < bound; ++i) {
      node = node->prev;
    }
  }

  return node;
}

static string_t run(struct CommandEntry entry) {
  PASS_NO_CLIENT(entry.client);

  if (entry.data->arg_count != 2) {
    return WRONG_ARGUMENT_ERROR("LINDEX");
  }

  const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

  if (!kv || kv->type != TELLY_LIST) {
    return INVALID_TYPE_ERROR("LINDEX");
  }

  const char *index_str = entry.data->args[1].value;

  if (!try_parse_integer(index_str)) {
    return RESP_ERROR_MESSAGE("Second argument must be an integer");
  }

  const struct List *list = kv->value;
  struct ListNode *node;

  if (index_str[0] != '-') {
    const uint64_t index = strtoull(index_str, (char **) NULL, 10);

    if (index == ULLONG_MAX) {
      return RESP_ERROR_MESSAGE("Index exceeded integer bounds");
    }

    node = get_node(list, index, FORWARD);
  } else {
    const uint64_t index = strtoull(index_str + 1, (char **) NULL, 10);

    if (index == ULLONG_MAX) {
      return RESP_ERROR_MESSAGE("Index exceeded integer bounds");
    }

    node = get_node(list, index, BACKWARD);
  }

  if (!node) {
    return RESP_NULL(entry.client->protover);
  }

  return write_value(node->value, node->type, entry.client->protover, entry.buffer);
}

const struct Command cmd_lindex = {
  .name = "LINDEX",
  .summary = "Returns element at the index in the list.",
  .since = "0.1.4",
  .complexity = "O(N) where N is min(absolute index, list size - absolute index - 1) number",
  .permissions = P_READ,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
