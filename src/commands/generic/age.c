#include <telly.h>

#include <stdio.h>
#include <stdint.h>
#include <time.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  uint32_t age;
  time_t start_at;
  get_server_time(&start_at, &age);
  age += difftime(time(NULL), start_at);

  const size_t nbytes = create_resp_integer(entry->buffer, age);
  return CREATE_STRING(entry->buffer, nbytes);
}

const struct Command cmd_age = {
  .name = "AGE",
  .summary = "Sends the server age as seconds.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .flags = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
