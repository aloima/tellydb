#include <telly.h>

#include <stdio.h>
#include <stdint.h>

#include <sys/time.h>

static string_t run(struct CommandEntry entry) {
  PASS_NO_CLIENT(entry.client);

  struct timeval timestamp;
  gettimeofday(&timestamp, NULL);

  const size_t nbytes = sprintf(entry.buffer, "*2\r\n:%ld\r\n:%ld\r\n", timestamp.tv_sec, timestamp.tv_usec);
  return CREATE_STRING(entry.buffer, nbytes);
}

const struct Command cmd_time = {
  .name = "TIME",
  .summary = "Returns the current server time.",
  .since = "0.1.2",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
