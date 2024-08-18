#include "../../headers/telly.h"

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration conf) {
  if (client != NULL) {
    switch (data->count) {
      case 1:
        if (client != NULL) write(client->connfd, "-ERR missing argument\r\n", 23);
        break;

      case 2:
        // TODO
        break;

      default:
        if (client != NULL) write(client->connfd, "-ERR additional argument(s)\r\n", 29);
        break;
    }
  }
}

struct Command cmd_get = {
  .name = "GET",
  .summary = "Gets value from specified key.",
  .run = run
};
