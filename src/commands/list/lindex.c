#include <telly.h>

static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count != 2) return;
  (void) insert_into_vector(server->keyspace, &entry->args->data[0]);
}



static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 2) {
    return WRONG_ARGUMENT_ERROR("LINDEX");
  }

  const struct KVPair *kv = get_data(entry->database, entry->args->data[0]);

  if (!kv || kv->type != TELLY_LIST) {
    return INVALID_TYPE_ERROR("LINDEX");
  }

  const char *index_str = entry->args->data[1].value;

  if (!try_parse_integer(index_str)) {
    return RESP_ERROR_MESSAGE("Second argument must be an integer");
  }

  LinkedList *list = kv->value;
  LinkedListNode *node;

  if (index_str[0] == '-') {
    uint64_t index = strtoull(index_str + 1, (char **) NULL, 10);
    index -= 1;

    if (index == ULLONG_MAX) {
      return RESP_ERROR_MESSAGE("Index exceeded integer bounds");
    }

    node = ll_get_from_index(list, index, LL_BACK);
  } else {
    const uint64_t index = strtoull(index_str, (char **) NULL, 10);

    if (index == ULLONG_MAX) {
      return RESP_ERROR_MESSAGE("Index exceeded integer bounds");
    }

    node = ll_get_from_index(list, index, LL_FRONT);
  }

  if (!node) {
    return RESP_NULL(entry->client->protover);
  }

  const DatabaseListNode *data = (DatabaseListNode *) node->data;
  return write_value(data->data, data->type, entry->client->protover, entry->client->write_buf);
}

const struct Command cmd_lindex = {
  .name = "LINDEX",
  .summary = "Returns element at the index in the list.",
  .since = "0.1.4",
  .complexity = "O(N) where N is min(absolute index, list size - absolute index - 1) number",
  .permissions = P_READ,
  .flags.value = CMD_FLAG_ACCESS_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
