#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <time.h>

static void run(struct CommandEntry entry) {
  if (!entry.client) return;

  uint32_t age;
  time_t start_at;
  get_server_time(&start_at, &age);
  age += difftime(time(NULL), start_at);

  char buf[24];
  const size_t nbytes = sprintf(buf, ":%d\r\n", age);
  _write(entry.client, buf, nbytes);
}

const struct Command cmd_age = {
  .name = "AGE",
  .summary = "Sends the server age as seconds.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
