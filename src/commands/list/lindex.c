#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/utils.h"

#include <stdlib.h>
#include <stdint.h>

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (client) {
    if (command->arg_count != 2) {
      WRONG_ARGUMENT_ERROR(client, "LINDEX", 4);
      return;
    }

    if (password->permissions & P_READ) {
      const struct KVPair *kv = get_data(command->args[0]);

      if (!kv || kv->type != TELLY_LIST) {
        _write(client, "-Value stored at the key is not a list\r\n", 40);
        return;
      }

      const char *index_str = command->args[1].value;

      if (!is_integer(index_str)) {
        _write(client, "-Second argument must be an integer\r\n", 37);
        return;
      }

      const struct List *list = kv->value;
      struct ListNode *node;

      if (index_str[0] != '-') {
        const uint32_t index = atoi(index_str);
        node = list->begin;

        if ((index + 1) > list->size) {
          WRITE_NULL_REPLY(client);
          return;
        }

        for (uint32_t i = 0; i < index; ++i) {
          node = node->next;
        }
      } else {
        const uint64_t index = atoi(index_str + 1);
        node = list->end;

        if (index > list->size) {
          WRITE_NULL_REPLY(client);
          return;
        }

        const uint32_t bound = index - 1;

        for (uint32_t i = 0; i < bound; ++i) {
          node = node->prev;
        }
      }

      write_value(client, node->value, node->type);
    } else {
      _write(client, "-Not allowed to use this command, need P_READ\r\n", 47);
    }
  }
}

const struct Command cmd_lindex = {
  .name = "LINDEX",
  .summary = "Returns element at the index in the list.",
  .since = "0.1.4",
  .complexity = "O(N) where N is index number",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
