#include <telly.h>

#include <stdint.h>
#include <stdbool.h>

static string_t run(struct CommandEntry entry) {
  PASS_NO_CLIENT(entry.client);

  if (entry.client->waiting_block) {
    return RESP_ERROR_MESSAGE("Already started a transaction block, cannot create one without executing before");
  }

  entry.client->waiting_block = reserve_transaction_block(entry.client, true);

  if (!entry.client->waiting_block) {
    return RESP_ERROR_MESSAGE("Cannot create a transaction block because of configuration limit");
  }

  return RESP_OK();
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
