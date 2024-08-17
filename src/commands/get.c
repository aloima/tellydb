#include "../../headers/telly.h"

#include <unistd.h>

static void run(int connfd, respdata_t *data, struct Configuration conf) {
  switch (data->count) {
    case 1:
      write(connfd, "-ERR missing argument\r\n", 23);
      break;

    case 2:
      // TODO
      break;

    default:
      write(connfd, "-ERR additional argument(s)\r\n", 29);
      break;
  }
}

struct Command cmd_get = {
  .name = "GET",
  .summary = "Gets value from specified key.",
  .run = run
};
