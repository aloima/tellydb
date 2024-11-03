#include "../../../headers/server.h"
#include "../../../headers/commands.h"
#include "../../../headers/utils.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

static void run(struct Client *client, commanddata_t *command, __attribute__((unused)) struct Password *password) {
  if (client) {
    switch (command->arg_count) {
      case 0:
        _write(client, "+PONG\r\n", 7);
        break;

      case 1: {
        const string_t arg = command->args[0];

        char buf[26 + arg.len];
        const size_t nbytes = sprintf(buf, "$%d\r\n%s\r\n", arg.len, arg.value);
        _write(client, buf, nbytes);

        break;
      }

      default:
        WRONG_ARGUMENT_ERROR(client, "PING", 4);
        break;
    }
  }
}

struct Command cmd_ping = {
  .name = "PING",
  .summary = "Pings the server and returns a simple/bulk string.",
  .since = "0.1.2",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
