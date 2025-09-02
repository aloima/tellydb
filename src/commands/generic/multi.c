#include <telly.h>

#include <stdint.h>

static void run(struct CommandEntry entry) {
  if (!entry.client) return;

  if (entry.client->waiting_execution) {
    WRITE_ERROR_MESSAGE(entry.client, "Already started a transaction block, cannot create one without executing before");
    return;
  }

  entry.client->waiting_execution = true;
  WRITE_OK(entry.client);
}

const struct Command cmd_multi = {
  .name = "MULTI",
  .summary = "Creates a transaction block consists of multiple transactions.",
  .since = "0.2.0",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
