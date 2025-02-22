#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

static void run(struct CommandEntry entry) {
  if (!entry.client) return;
  if (entry.data->arg_count == 0) {
    MISSING_SUBCOMMAND_ERROR(entry.client, "COMMAND", 7);
    return;
  }

  const string_t input = entry.data->args[0];
  char subcommand[input.len + 1];
  to_uppercase(input.value, subcommand);

  if (streq("DOCS", subcommand)) {
    const struct Command *commands = get_commands();
    const uint32_t command_count = get_command_count();

    char res[16384];
    uint32_t res_len;

    switch (entry.client->protover) {
      case RESP2:
        res_len = sprintf(res, "*%d\r\n", command_count * 2);

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

        break;

      case RESP3:
        res_len = sprintf(res, "%%%d\r\n", command_count);

        for (uint32_t i = 0; i < command_count; ++i) {
          struct Command command = commands[i];

          char buf[4096];
          res_len += sprintf(buf, (
            "$%ld\r\n%s\r\n"
            "%%3\r\n"
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

        break;

      default:
        res_len = 0;
    }

    _write(entry.client, res, res_len);
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

    _write(entry.client, res, res_len);
  } else if (streq("COUNT", subcommand)) {
    char buf[14];
    const size_t nbytes = sprintf(buf, ":%d\r\n", get_command_count());
    _write(entry.client, buf, nbytes);
  } else {
    INVALID_SUBCOMMAND_ERROR(entry.client, "COMMAND", 7);
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

const struct Command cmd_command = {
  .name = "COMMAND",
  .summary = "Gives information about the commands in the server.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .subcommands = subcommands,
  .subcommand_count = 3,
  .run = run,
};
