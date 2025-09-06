#include <telly.h>

#include <stddef.h>

static string_t run(struct CommandEntry entry) {
  if (entry.data->arg_count != 1) {
    PASS_NO_CLIENT(entry.client);
    return WRONG_ARGUMENT_ERROR("RPOP");
  }

  const string_t key = entry.data->args[0];
  const struct KVPair *kv = get_data(entry.database, key);

  if (!kv) {
    PASS_NO_CLIENT(entry.client);
    return RESP_NULL(entry.client->protover);
  }

  if (kv->type != TELLY_LIST) {
    PASS_NO_CLIENT(entry.client);
    return INVALID_TYPE_ERROR("RPOP");
  }

  struct List *list = kv->value;
  struct ListNode *node = list->end;
  string_t response = EMPTY_STRING();

  if (entry.client) {
    return write_value(list->end->value, list->end->type, entry.client->protover, entry.buffer);
  }

  if (list->size == 1) {
    delete_data(entry.database, key);
  } else {
    list->end = list->end->prev;
    list->end->next = NULL;

    list->size -= 1;
    free_listnode(node);
  }

  return response;
}

const struct Command cmd_rpop = {
  .name = "RPOP",
  .summary = "Removes and returns last element of the list.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .permissions = (P_READ | P_WRITE),
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
