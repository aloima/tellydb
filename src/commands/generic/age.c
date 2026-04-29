#include <telly.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  uint32_t age;
  time_t start_at;
  get_server_time(&start_at, &age);

  const time_t current_time = time(NULL);
  if (current_time == INVALID_TIME)
    return RESP_ERROR_MESSAGE("time() system call is failed");

  age += difftime(current_time, start_at);

  const size_t nbytes = create_resp_integer(entry->client->write_buf, age);
  return CREATE_STRING(entry->client->write_buf, nbytes);
}

const struct Command cmd_age = {
  .name = "AGE",
  .summary = "Sends the server age as seconds.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .flags.value = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
