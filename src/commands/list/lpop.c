#include "../../../headers/telly.h"

#include <stddef.h>

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (command->arg_count != 1) {
    if (client) WRONG_ARGUMENT_ERROR(client, "LPOP", 4);
    return;
  }

  if(password->permissions & (P_READ | P_WRITE)) {
    const string_t key = command->args[0];
    const struct KVPair *kv = get_data(key);

    if (kv) {
      if (kv->type != TELLY_LIST) {
        if (client) _write(client, "-Value stored at the key is not a list\r\n", 40);
        return;
      }

      struct List *list = kv->value;
      struct ListNode *node = list->begin;

      if (client) write_value(client, node->value, node->type);

      if (list->size == 1) {
        delete_data(key);
      } else {
        list->begin = list->begin->next;
        list->begin->prev = NULL;

        list->size -= 1;
        free_listnode(node);
      }
    } else if (client) WRITE_NULL_REPLY(client);
  } else if (client) {
    _write(client, "-Not allowed to use this command, need P_READ and P_WRITE\r\n", 59);
  }
}

const struct Command cmd_lpop = {
  .name = "LPOP",
  .summary = "Removes and returns first element of the list.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
