#include "../../../headers/server.h"
#include "../../../headers/commands.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count == 1) {
      uint64_t age;
      time_t start_at;
      get_server_time(&start_at, &age);
      age += difftime(time(NULL), start_at);

      char buf[24];
      const size_t nbytes = sprintf(buf, ":%ld\r\n", age);
      _write(client, buf, nbytes);
    } else {
      WRONG_ARGUMENT_ERROR(client, "AGE", 3);
    }
  }
}

struct Command cmd_age = {
  .name = "AGE",
  .summary = "Sends the server age as seconds.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
