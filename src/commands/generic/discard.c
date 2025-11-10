#include <telly.h>

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (!entry->client->waiting_block) {
    return RESP_ERROR_MESSAGE("A transaction block did not started, cannot execute one without starting before");
  }

  const uint64_t count = entry->client->waiting_block->transaction_count;
  remove_transaction_block(entry->client->waiting_block, false);
  entry->client->waiting_block = NULL;

  const size_t nbytes = create_resp_integer(entry->buffer, count);
  return CREATE_STRING(entry->buffer, nbytes);
}

const struct Command cmd_discard = {
  .name = "DISCARD",
  .summary = "Discards the current started transaction block.",
  .since = "0.2.0",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
