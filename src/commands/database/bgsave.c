#include <telly.h>

static string_t run(struct CommandEntry *entry) {
  (void) entry;

  const time_t current_time = time(NULL);
  if (current_time == INVALID_TIME)
    return RESP_ERROR_MESSAGE("time() system call is failed");

  uint32_t age = server->age;
  age += difftime(current_time, server->start_at);

  const BackgroundSavingStatus response = bg_save(age);

  switch (response) {
    case BGSAVE_ALREADY_SAVING:
      return RESP_ERROR_MESSAGE("Saving process is already active in background");

    case BGSAVE_THREAD_FAILED:
      return RESP_ERROR_MESSAGE("Cannot open external background saving thread");

    case BGSAVE_SUCCESSFUL:
      return RESP_OK();
  }
}

const struct Command cmd_bgsave = {
  .name = "BGSAVE",
  .summary = "Saves all data to database file in background using a thread.",
  .since = "0.1.6",
  .complexity = "O(N) where N is cached key-value pair count",
  .permissions = P_SERVER,
  .flags.value = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = NULL
};
