#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/btree.h"
#include "../../../headers/utils.h"

#include <stddef.h>

static void run(struct Client *client, respdata_t *data) {
  if (client && data->count != 2) {
    WRONG_ARGUMENT_ERROR(client, "LPOP", 4);
    return;
  }

  const char *key = data->value.array[1]->value.string.value;
  const struct KVPair *kv = get_data(key);

  if (kv) {
    if (client && kv->type != TELLY_LIST) {
      _write(client, "-Value stored at the key is not a list\r\n", 40);
      return;
    }

    struct List *list = kv->value;
    struct ListNode *node = list->begin;

    if (client) write_value(client, node->value, node->type);

    if (list->size == 1) {
      delete_kv_from_btree(get_cache(), key);
    } else {
      list->begin = list->begin->next;
      list->begin->prev = NULL;

      list->size -= 1;
      free_listnode(node);
    }
  } else if (client) WRITE_NULL_REPLY(client);
}

struct Command cmd_lpop = {
  .name = "LPOP",
  .summary = "Removes and returns first element(s) of the list stored at the key.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
