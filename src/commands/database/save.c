#include <telly.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  uint32_t server_age;
  time_t start_at;
  get_server_time(&start_at, &server_age);

  const time_t current_time = time(NULL);
  if (current_time == INVALID_TIME)
    return RESP_ERROR_MESSAGE("time() system call is failed");

  server_age += difftime(current_time, start_at);

  if (save_data(server_age) != -1) {
    return RESP_OK();
  } else {
    return RESP_ERROR_MESSAGE("Saving data is failed");
  }
}

const struct Command cmd_save = {
  .name = "SAVE",
  .summary = "Saves all data in all databases to the database file.",
  .since = "0.1.6",
  .complexity = "O(N) where N is number of the keys in all databases",
  .permissions = P_SERVER,
  .flags.value = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = NULL
};
