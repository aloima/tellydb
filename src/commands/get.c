#include "../telly.h"

#include <unistd.h>

static void run(int connfd, respdata_t data) {
  switch (data.count) {
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
  .run = run
};
