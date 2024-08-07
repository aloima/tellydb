#include "../telly.h"

#include <unistd.h>

void cmd_command(int connfd, respdata_t data) {
  if (data.count != 1) {
    char *subcommand = data.value.array[1].value.string.data;

    if (streq("DOCS", subcommand)) {
      write(connfd, "*0\r\n", 4);
    }
  }
}
