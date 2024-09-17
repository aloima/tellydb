#include "../../headers/telly.h"

#include <stddef.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (client) {
    if (data->count != 3) {
      write(client->connfd, "-Wrong argument count for 'HTYPE' command\r\n", 43);
      return;
    }

    char *key = data->value.array[1]->value.string.value;
    char *name = data->value.array[2]->value.string.value;

    struct KVPair *kv = get_data(key, conf);

    if (kv && kv->type == TELLY_HASHTABLE) {
      struct FVPair *fv = get_fv_from_hashtable(kv->value.hashtable, name);

      switch (fv->type) {
        case TELLY_NULL:
          write(client->connfd, "+null\r\n", 7);
          break;

        case TELLY_INT:
          write(client->connfd, "+integer\r\n", 10);
          break;

        case TELLY_STR:
          write(client->connfd, "+string\r\n", 9);
          break;

        case TELLY_BOOL:
          write(client->connfd, "+boolean\r\n", 10);
          break;

        default:
          break;
      }
    } else {
      write(client->connfd, "$-1\r\n", 5);
      return;
    }
  }
}

struct Command cmd_htype = {
  .name = "HTYPE",
  .summary = "Returns type of field from hash table of key.",
  .since = "1.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
