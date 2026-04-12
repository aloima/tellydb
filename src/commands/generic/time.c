#include <telly.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  struct timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    return RESP_ERROR_MESSAGE("clock_gettime() syscall error");

  const intmax_t secs = ts.tv_sec;
  const intmax_t usecs = (ts.tv_nsec / 1000);

  const size_t nbytes = sprintf(entry->client->write_buf, (
    "*2\r\n"
      ":%" PRIiMAX "\r\n"
      ":%" PRIiMAX "\r\n"
  ), secs, usecs);

  return CREATE_STRING(entry->client->write_buf, nbytes);
}

const struct Command cmd_time = {
  .name = "TIME",
  .summary = "Returns the current server time.",
  .since = "0.1.2",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .flags.value = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
