#include "../../../headers/telly.h"

#include <stddef.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (client && data->count != 2) {
    WRONG_ARGUMENT_ERROR(client->connfd, "LPOP", 4);
    return;
  }

  string_t key = data->value.array[1]->value.string;
  struct KVPair *pair = get_data(key.value, conf);

  if (pair) {
    if (client && pair->type != TELLY_LIST) {
      write(client->connfd, "-Value stored at the key is not a list\r\n", 40);
      return;
    }

    struct List *list = pair->value.list;
    struct ListNode *node = list->begin;

    if (client) write_value(client->connfd, node->value, node->type);

    if (list->size == 1) {
      free_list(list);
      // delete from btree
    } else {
      list->begin = list->begin->next;
      list->begin->prev = NULL;

      list->size -= 1;
      free_listnode(node);
    }
  } else {
    write(client->connfd, "$-1\r\n", 5);
  }
}

struct Command cmd_lpop = {
  .name = "LPOP",
  .summary = "Removes and returns first element(s) of the list stored at the key.",
  .since = "1.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
