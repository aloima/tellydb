#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stdlib.h>
#include <stdint.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count != 3) {
      WRONG_ARGUMENT_ERROR(client, "LINDEX", 4);
      return;
    }

    const char *key = data->value.array[1]->value.string.value;
    const struct KVPair *kv = get_data(key);

    if (!kv || kv->type != TELLY_LIST) {
      _write(client, "-Value stored at the key is not a list\r\n", 40);
      return;
    }

    const char *index_str = data->value.array[2]->value.string.value;

    if (!is_integer(index_str)) {
      _write(client, "-Second argument must be an integer\r\n", 37);
      return;
    }

    const struct List *list = kv->value->list;
    struct ListNode *node;

    if (index_str[0] != '-') {
      const uint32_t index = atoi(index_str);
      node = list->begin;

      if ((index + 1) > list->size) {
        _write(client, "$-1\r\n", 5);
        return;
      }

      for (uint32_t i = 0; i < index; ++i) {
        node = node->next;
      }
    } else {
      const uint64_t index = atoi(index_str + 1);
      node = list->end;

      if (index > list->size) {
        _write(client, "$-1\r\n", 5);
        return;
      }

      const uint32_t bound = index - 1;

      for (uint32_t i = 0; i < bound; ++i) {
        node = node->prev;
      }
    }

    write_value(client, node->value, node->type);
  }
}

struct Command cmd_lindex = {
  .name = "LINDEX",
  .summary = "Returns element at the index in the list stored at the key.",
  .since = "0.1.4",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
