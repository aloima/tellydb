#include "../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, __attribute__((unused)) struct Configuration *conf) {
  if (client != NULL) {
    switch (data->count) {
      case 1:
        write(client->connfd, "+PONG\r\n", 7);
        break;

      case 2: {
        string_t arg = data->value.array[1]->value.string;

        const uint32_t buf_len = 5 + get_digit_count(arg.len) + arg.len;
        char buf[buf_len + 1];
        sprintf(buf, "$%ld\r\n%s\r\n", arg.len, arg.value);
        write(client->connfd, buf, buf_len);

        break;
      }

      default:
        write(client->connfd, "-Wrong argument count for 'PING' command\r\n", 42);
        break;
    }
  }
}

struct Command cmd_ping = {
  .name = "PING",
  .summary = "Pings the server and returns a simple/bulk string.",
  .since = "1.0.2",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
