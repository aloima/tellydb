#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data) {
  if (data->count != 1 && client) {
    string_t input = data->value.array[1]->value.string;
    char subcommand[input.len + 1];
    to_uppercase(input.value, subcommand);

    if (streq("DOCS", subcommand)) {
      const struct Command *commands = get_commands();
      const uint32_t command_count = get_command_count();

      char res[16384];
      uint32_t res_len = sprintf(res, "*%d\r\n", command_count * 2);

      for (uint32_t i = 0; i < command_count; ++i) {
        struct Command command = commands[i];

        char buf[4096];
        res_len += sprintf(buf, (
          "$%ld\r\n%s\r\n"
          "*6\r\n"
            "$7\r\nsummary\r\n"
            "$%ld\r\n%s\r\n"

            "$5\r\nsince\r\n"
            "$%ld\r\n%s\r\n"

            "$10\r\ncomplexity\r\n"
            "$%ld\r\n%s\r\n"
        ),
          strlen(command.name), command.name,
          strlen(command.summary), command.summary,
          strlen(command.since), command.since,
          strlen(command.complexity), command.complexity
        );

        strcat(res, buf);
      }

      write(client->connfd, res, res_len);
    } else if (streq("LIST", subcommand)) {
      const struct Command *commands = get_commands();
      const uint32_t command_count = get_command_count();

      char res[16384];
      uint32_t res_len = sprintf(res, "*%d\r\n", command_count);

      for (uint32_t i = 0; i < command_count; ++i) {
        char buf[128];
        res_len += sprintf(buf, "+%s\r\n", commands[i].name);
        strcat(res, buf);
      }

      write(client->connfd, res, res_len);
    } else if (streq("COUNT", subcommand)) {
      const uint32_t command_count = get_command_count();
      const uint32_t res_len = 3 + get_digit_count(command_count);
      char res[res_len + 1];

      sprintf(res, ":%d\r\n", get_command_count());
      write(client->connfd, res, res_len);
    }
  }
}

static struct Subcommand subcommands[] = {
  (struct Subcommand) {
    .name = "LIST",
    .summary = "Returns name list of all commands.",
    .since = "0.1.0",
    .complexity = "O(N) where N is count of all commands"
  },
  (struct Subcommand) {
    .name = "COUNT",
    .summary = "Returns count of all commands in the server.",
    .since = "0.1.0",
    .complexity = "O(1)"
  },
  (struct Subcommand) {
    .name = "DOCS",
    .summary = "Returns documentation about multiple commands.",
    .since = "0.1.0",
    .complexity = "O(N) where N is count of commands to look up"
  }
};

struct Command cmd_command = {
  .name = "COMMAND",
  .summary = "Gives information about the commands in the server.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = subcommands,
  .subcommand_count = 3,
  .run = run,
};
