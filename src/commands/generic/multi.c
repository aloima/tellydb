#include <telly.h>

#include <stdint.h>
#include <stdbool.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->client->waiting_block) {
    return RESP_ERROR_MESSAGE("Already started a transaction block, cannot create one without executing before");
  }

  entry->client->waiting_block = malloc(sizeof(TransactionBlock));

  if (entry->client->waiting_block == NULL) {
    return RESP_ERROR_MESSAGE("Cannot create a transaction block, out of memory");
  }

  TransactionBlock *block = entry->client->waiting_block;
  block->type = TX_WAITING;
  block->client = entry->client;
  block->password = entry->password;
  memset(&block->data, 0, sizeof(block->data));

  return RESP_OK();
}

const struct Command cmd_multi = {
  .name = "MULTI",
  .summary = "Creates a transaction block consists of multiple transactions.",
  .since = "0.2.0",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .flags.value = CMD_FLAG_WAITING_TX,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
