#include "../../../headers/telly.h"

#include <stddef.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (client && data->count != 2) {
    WRONG_ARGUMENT_ERROR(client->connfd, "RPOP", 4);
    return;
  }

  const string_t key = data->value.array[1]->value.string;
  struct KVPair *pair = get_data(key.value, conf);

  if (pair) {
    if (client && pair->type != TELLY_LIST) {
      write(client->connfd, "-Value stored at the key is not a list\r\n", 40);
      return;
    }

    struct List *list = pair->value.list;
    struct ListNode *node = list->end;

    if (client) write_value(client->connfd, list->end->value, list->end->type);

    if (list->size == 1) {
      free_list(list);
      // delete from btree
    } else {
      list->end = list->end->prev;
      list->end->next = NULL;
    }

    list->size -= 1;
    free_listnode(node);
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
