#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/utils.h"

#include <stddef.h>

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (command->arg_count != 1) {
    if (client) WRONG_ARGUMENT_ERROR(client, "RPOP", 4);
    return;
  }

  if (password->permissions & (P_READ & P_WRITE)) {
    const char *key = command->args[0].value;
    struct KVPair *kv = get_data(key);

    if (kv) {
      if (kv->type != TELLY_LIST) {
        if (client) _write(client, "-Value stored at the key is not a list\r\n", 40);
        return;
      }

      struct List *list = kv->value;
      struct ListNode *node = list->end;

      if (client) write_value(client, list->end->value, list->end->type);

      if (list->size == 1) {
        // TODO: complete deletion of the list
        delete_kv_from_cache(key);
      } else {
        list->end = list->end->prev;
        list->end->next = NULL;

        list->size -= 1;
        free_listnode(node);
      }
    } else if (client) WRITE_NULL_REPLY(client);
  } else if (client) {
    _write(client, "-Not allowed to use this command, need P_READ and P_WRITE\r\n", 59);
  }
}

const struct Command cmd_rpop = {
  .name = "RPOP",
  .summary = "Removes and returns last element of the list.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
