#include "../telly.h"

#include <stdio.h>
#include <stdint.h>

#include <unistd.h>

static void run(int connfd, respdata_t data) {
  if (data.count != 1) {
    char *subcommand = data.value.array[1].value.string.data;

    if (streq("DOCS", subcommand)) {
      write(connfd, "*0\r\n", 4);
    } else if (streq("LIST", subcommand)) {
      struct Command *commands = get_commands();
      uint32_t command_count = get_command_count();

      char res[16384];
      sprintf(res, "*%d\r\n", command_count);

      for (uint32_t i = 0; i < command_count; ++i) {
        char buf[128];
        sprintf(buf, "+%s\r\n", commands[i].name);
        strcat(res, buf);
      }

      write(connfd, res, strlen(res));
    }
  }
}

struct Command cmd_command = {
  .name = "COMMAND",
  .run = run
};
