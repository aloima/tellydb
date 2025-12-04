#include <telly.h>

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include <sys/time.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  struct timeval timestamp;
  gettimeofday(&timestamp, NULL);

  const size_t nbytes = sprintf(entry->buffer, "*2\r\n"
    ":%" PRIiMAX "\r\n"
    ":%" PRIiMAX "\r\n", (intmax_t) timestamp.tv_sec, (intmax_t) timestamp.tv_usec);

  return CREATE_STRING(entry->buffer, nbytes);
}

const struct Command cmd_time = {
  .name = "TIME",
  .summary = "Returns the current server time.",
  .since = "0.1.2",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .flags = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
