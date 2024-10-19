#include "../../../headers/server.h"
#include "../../../headers/commands.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include <sys/time.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count != 1) {
      WRONG_ARGUMENT_ERROR(client, "TIME", 4);
      return;
    }

    struct timeval timestamp;
    gettimeofday(&timestamp, NULL);

    char buf[51];
    const size_t nbytes = sprintf(buf, "*2\r\n:%ld\r\n:%ld\r\n", timestamp.tv_sec, timestamp.tv_usec);
    _write(client, buf, nbytes);
  }
}

struct Command cmd_time = {
  .name = "TIME",
  .summary = "Returns the current server time as two elements in a array, a Unix timestamp and microseconds already elapsed in the current second.",
  .since = "0.1.2",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
