#include <telly.h>

#include <stddef.h>

static void run(struct CommandEntry entry) {
  if (entry.data->arg_count != 1) {
    if (entry.client) {
      WRONG_ARGUMENT_ERROR(entry.client, "RPOP");
    }

    return;
  }

  const string_t key = entry.data->args[0];
  const struct KVPair *kv = get_data(entry.database, key);

  if (!kv) {
    if (entry.client) WRITE_NULL_REPLY(entry.client);
    return;
  }

  if (kv->type != TELLY_LIST) {
    if (entry.client) {
      INVALID_TYPE_ERROR(entry.client, "RPOP");
    }

    return;
  }

  struct List *list = kv->value;
  struct ListNode *node = list->end;

  if (entry.client) {
    write_value(entry.client, list->end->value, list->end->type);
  }

  if (list->size == 1) {
    delete_data(entry.database, key);
  } else {
    list->end = list->end->prev;
    list->end->next = NULL;

    list->size -= 1;
    free_listnode(node);
  }
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
