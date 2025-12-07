#include <telly.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->data->arg_count == 0) {
    return MISSING_SUBCOMMAND_ERROR("COMMAND");
  }

  const string_t subcommand = entry->data->args[0];
  to_uppercase(subcommand, subcommand.value);

  string_t response;

  if (streq("DOCS", subcommand.value)) {
    const struct Command *commands = get_commands();
    const uint32_t command_count = get_command_count();

    char *res = entry->client->write_buf;
    uint32_t res_len;

    switch (entry->client->protover) {
      case RESP2:
        res_len = sprintf(res, "*%" PRIu32 "\r\n", command_count * 2);

        for (uint32_t i = 0; i < command_count; ++i) {
          struct Command command = commands[i];

          char buf[4096];
          res_len += sprintf(buf, (
            "$%zu\r\n%s\r\n"
            "*6\r\n"
              "$7\r\nsummary\r\n"
              "$%zu\r\n%s\r\n"

              "$5\r\nsince\r\n"
              "$%zu\r\n%s\r\n"

              "$10\r\ncomplexity\r\n"
              "$%zu\r\n%s\r\n"
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
        res_len = sprintf(res, "%%%" PRIu32 "\r\n", command_count);

        for (uint32_t i = 0; i < command_count; ++i) {
          struct Command command = commands[i];

          char buf[4096];
          res_len += sprintf(buf, (
            "$%zu\r\n%s\r\n"
            "%%3\r\n"
              "$7\r\nsummary\r\n"
              "$%zu\r\n%s\r\n"

              "$5\r\nsince\r\n"
              "$%zu\r\n%s\r\n"

              "$10\r\ncomplexity\r\n"
              "$%zu\r\n%s\r\n"
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

    response = CREATE_STRING(res, res_len);
  } else if (streq("LIST", subcommand.value)) {
    const struct Command *commands = get_commands();
    const uint32_t command_count = get_command_count();

    char *res = entry->client->write_buf;
    uint32_t res_len = sprintf(res, "*%" PRIu32 "\r\n", command_count);

    for (uint32_t i = 0; i < command_count; ++i) {
      char buf[128];
      res_len += sprintf(buf, "+%s\r\n", commands[i].name);
      strcat(res, buf);
    }

    response = CREATE_STRING(res, res_len);
  } else if (streq("COUNT", subcommand.value)) {
    const size_t nbytes = sprintf(entry->client->write_buf, ":%" PRIu32 "\r\n", get_command_count());
    response = CREATE_STRING(entry->client->write_buf, nbytes);
  } else {
    response = INVALID_SUBCOMMAND_ERROR("COMMAND");
  }

  return response;
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
  .flags = CMD_FLAG_NO_FLAG,
  .subcommands = subcommands,
  .subcommand_count = 3,
  .run = run,
};
