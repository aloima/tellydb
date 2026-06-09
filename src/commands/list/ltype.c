#include <telly.h>

static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count != 2) return;
  (void) insert_into_vector(server->keyspace, &entry->args->data[0]);
}



static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 2) {
    return WRONG_ARGUMENT_ERROR("LTYPE");
  }

  const KeyValue *kv = get_data(entry->database, entry->args->data[0]);

  if (!kv || kv->value.type != TELLY_LIST) {
    return INVALID_TYPE_ERROR("LTYPE");
  }

  const string_t index_str = entry->args->data[1];

  if (!try_parse_integer(index_str)) {
    return RESP_ERROR_MESSAGE("Second argument must be an integer");
  }

  LinkedList *list = kv->value.data;
  LinkedListNode *node;

  if (index_str.value[0] == '-') {
    index_str.value[0] = '+'; // atoull_s throws EINVAL on '-'

    uint64_t index = atoull_s(index_str);
    ASSERT(errno, !=, EINVAL); // try_parse_integer prevents it

    index -= 1;

    if (errno == ERANGE) {
      return RESP_ERROR_MESSAGE("Index exceeded integer bounds");
    }

    node = ll_get_from_index(list, index, LL_BACK);
  } else {
    const uint64_t index = atoull_s(index_str);
    ASSERT(errno, !=, EINVAL); // try_parse_integer prevents it

    if (errno == ERANGE) {
      return RESP_ERROR_MESSAGE("Index exceeded integer bounds");
    }

    node = ll_get_from_index(list, index, LL_FRONT);
  }

  if (!node) {
    return RESP_NULL(entry->client->protover);
  }

  const Value *data = (Value *) node->data;
  return get_resp_type_name(data->type);
}

const struct Command cmd_ltype = {
  .name = "LTYPE",
  .summary = "Returns type of the element at the index in the list.",
  .since = "1.0.0",
  .complexity = "O(N) where N is min(absolute index, list size - absolute index - 1) number",
  .permissions = P_READ,
  .flags.value = CMD_FLAG_ACCESS_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
