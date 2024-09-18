#include "../../headers/telly.h"

#include <stddef.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (client) {
    if (data->count == 2) {
      char *key = data->value.array[1]->value.string.value;
      struct KVPair *res = get_data(key, conf);

      if (res) {
        switch (res->type) {
          case TELLY_NULL:
            write(client->connfd, "+null\r\n", 7);
            break;

          case TELLY_INT:
            write(client->connfd, "+integer\r\n", 10);
            break;

          case TELLY_STR:
            write(client->connfd, "+string\r\n", 9);
            break;

          case TELLY_HASHTABLE:
            write(client->connfd, "+hash table\r\n", 13);
            break;

          case TELLY_LIST:
            write(client->connfd, "+a list\r\n", 9);
            break;

          case TELLY_BOOL:
            write(client->connfd, "+boolean\r\n", 10);
            break;
        }
      } else {
        write(client->connfd, "$-1\r\n", 5);
      }
    } else {
      write(client->connfd, "-invalid argument usage\r\n", 25);
    }
  }
}

struct Command cmd_type = {
  .name = "TYPE",
  .summary = "Returns type of value of key.",
  .since = "1.0.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
