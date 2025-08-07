#include <telly.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static void run(struct CommandEntry entry) {
  if (!entry.client) return;
  switch (entry.data->arg_count) {
    case 0:
      _write(entry.client, "+PONG\r\n", 7);
      break;

    case 1: {
      const string_t arg = entry.data->args[0];

      char *buf = malloc(26 + arg.len);
      const size_t nbytes = sprintf(buf, "$%u\r\n%s\r\n", arg.len, arg.value);
      _write(entry.client, buf, nbytes);
      free(buf);

      break;
    }

    default:
      WRONG_ARGUMENT_ERROR(entry.client, "PING", 4);
  }
}

const struct Command cmd_ping = {
  .name = "PING",
  .summary = "Pings the server and returns a simple/bulk string.",
  .since = "0.1.2",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
