#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/btree.h"

#include <stddef.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data) {
  if (client && data->count != 2) {
    WRONG_ARGUMENT_ERROR(client->connfd, "RPOP", 4);
    return;
  }

  const string_t key = data->value.array[1]->value.string;
  struct KVPair *kv = get_data(key.value);

  if (kv) {
    if (client && kv->type != TELLY_LIST) {
      write(client->connfd, "-Value stored at the key is not a list\r\n", 40);
      return;
    }

    struct List *list = kv->value->list;
    struct ListNode *node = list->end;

    if (client) write_value(client->connfd, list->end->value, list->end->type);

    if (list->size == 1) {
      delete_kv_from_btree(get_cache(), key.value);
    } else {
      list->end = list->end->prev;
      list->end->next = NULL;

      list->size -= 1;
      free_listnode(node);
    }
  } else {
    write(client->connfd, "$-1\r\n", 5);
  }
}

struct Command cmd_rpop = {
  .name = "RPOP",
  .summary = "Removes and returns last element(s) of the list stored at the key.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
