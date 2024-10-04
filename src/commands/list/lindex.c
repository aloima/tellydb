#include "../../../headers/telly.h"

#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count != 3) {
      WRONG_ARGUMENT_ERROR(client->connfd, "LINDEX", 4);
      return;
    }

    string_t key = data->value.array[1]->value.string;
    struct KVPair *pair = get_data(key.value);

    if (!pair || pair->type != TELLY_LIST) {
      write(client->connfd, "-Value stored at the key is not a list\r\n", 40);
      return;
    }

    char *index_str = data->value.array[2]->value.string.value;

    if (!is_integer(index_str)) {
      write(client->connfd, "-Second argument must be an integer\r\n", 37);
      return;
    }

    struct List *list = pair->value->list;
    struct ListNode *node;

    if (index_str[0] != '-') {
      const uint64_t index = atoi(index_str);
      node = list->begin;

      if ((index + 1) > list->size) {
        write(client->connfd, "$-1\r\n", 5);
        return;
      }

      for (uint64_t i = 0; i < index; ++i) {
        node = node->next;
      }
    } else {
      const uint64_t index = atoi(index_str + 1);
      node = list->end;

      if (index > list->size) {
        write(client->connfd, "$-1\r\n", 5);
        return;
      }

      const uint64_t bound = index - 1;

      for (uint64_t i = 0; i < bound; ++i) {
        node = node->prev;
      }
    }

    write_value(client->connfd, node->value, node->type);
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
