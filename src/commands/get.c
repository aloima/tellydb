#include "../../headers/telly.h"

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration conf) {
  if (client != NULL) {
    switch (data->count) {
      case 1:
        write(client->connfd, "-missing argument\r\n", 19);
        break;

      case 2:
        // TODO
        break;

      default:
        write(client->connfd, "-additional argument(s)\r\n", 25);
        break;
    }
  }
}

struct Command cmd_get = {
  .name = "GET",
  .summary = "Gets value from specified key.",
  .run = run
};
