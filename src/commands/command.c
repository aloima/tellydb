#include "../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>

static void run(int connfd, respdata_t data, struct Configuration conf) {
  if (data.count != 1) {
    char *subcommand = data.value.array[1].value.string.value;

    if (streq("DOCS", subcommand)) {
      struct Command *commands = get_commands();
      const uint32_t command_count = get_command_count();

      char res[16384];
      sprintf(res, "*%d\r\n", command_count * 2);

      for (uint32_t i = 0; i < command_count; ++i) {
        struct Command command = commands[i];

        char buf[128];
        sprintf(buf, "$%ld\r\n%s\r\n*2\r\n$7\r\nsummary\r\n$%ld\r\n%s\r\n", strlen(command.name), command.name, strlen(command.summary), command.summary);
        strcat(res, buf);
      }

      write(connfd, res, strlen(res));
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
    } else if (streq("COUNT", subcommand)) {
      char res[8];
      sprintf(res, ":%d\r\n", get_command_count());
      write(connfd, res, strlen(res));
    }
  }
}

struct Command cmd_command = {
  .name = "COMMAND",
  .summary = "Gives detailed information about the commands.",
  .run = run
};
