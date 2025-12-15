#include <telly.h>

#include <stdint.h>
#include <stdbool.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (!entry->client->waiting_block) {
    return RESP_ERROR_MESSAGE("A transaction block did not started, cannot execute one without starting before");
  }

  entry->client->waiting_block->type = TX_MULTIPLE;
  TransactionBlock *queued = enqueue_to_transaction_queue(&entry->client->waiting_block);

  if (queued == NULL) {
    return RESP_ERROR_MESSAGE("The transaction block could not be executed, out of memory");
  }

  entry->client->waiting_block = NULL;
  PASS_COMMAND();
}

const struct Command cmd_exec = {
  .name = "EXEC",
  .summary = "Executes a transaction block consists of multiple transactions.",
  .since = "0.2.0",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .flags = CMD_FLAG_WAITING_TX,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
