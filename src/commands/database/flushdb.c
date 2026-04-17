#include <telly.h>

static string_t run(struct CommandEntry *entry) {
  clear_database(entry->database);

  PASS_NO_CLIENT(entry->client);
  return RESP_OK();
}

const struct Command cmd_flushdb = {
  .name = "FLUSHDB",
  .summary = "Deletes all the keys of the currently selected database.",
  .since = "1.0.0",
  .complexity = "O(N) where N is number of the keys in the currently selected database",
  .permissions = P_WRITE,
  .flags.value = CMD_FLAG_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
