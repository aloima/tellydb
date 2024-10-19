#include "../../../headers/server.h"
#include "../../../headers/commands.h"
#include "../../../headers/utils.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    switch (data->count) {
      case 1:
        _write(client, "+PONG\r\n", 7);
        break;

      case 2: {
        const string_t arg = data->value.array[1]->value.string;

        char buf[26 + arg.len];
        const size_t nbytes = sprintf(buf, "$%ld\r\n%s\r\n", arg.len, arg.value);
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
