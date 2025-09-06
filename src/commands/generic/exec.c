#include <telly.h>

#include <stdint.h>
#include <stdbool.h>

static string_t run(struct CommandEntry entry) {
  PASS_NO_CLIENT(entry.client);

  if (!entry.client->waiting_block) {
    return RESP_ERROR_MESSAGE("A transaction block did not started, cannot execute one without starting before");
  }

  release_queued_transaction_block(entry.client);
  PASS_COMMAND();
}

const struct Command cmd_exec = {
  .name = "EXEC",
  .summary = "Executes a transaction block consists of multiple transactions.",
  .since = "0.2.0",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
