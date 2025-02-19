#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>

#include <sys/time.h>
#include <bits/types/struct_timeval.h>

static void run(struct CommandEntry entry) {
  if (entry.client) {
    struct timeval timestamp;
    gettimeofday(&timestamp, NULL);

    char buf[51];
    const size_t nbytes = sprintf(buf, "*2\r\n:%ld\r\n:%ld\r\n", timestamp.tv_sec, timestamp.tv_usec);
    _write(entry.client, buf, nbytes);
  }
}

const struct Command cmd_time = {
  .name = "TIME",
  .summary = "Returns the current server time.",
  .since = "0.1.2",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
