#include <telly.h>

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (!entry->client->waiting_block) {
    return RESP_ERROR_MESSAGE("There is no transaction block, cannot execute one without starting before");
  }

  const uint64_t count = entry->client->waiting_block->data.multiple.transaction_count;
  remove_transaction_block(entry->client->waiting_block);
  entry->client->waiting_block = NULL;

  const size_t nbytes = create_resp_integer(entry->client->write_buf, count);
  return CREATE_STRING(entry->client->write_buf, nbytes);
}

const struct Command cmd_discard = {
  .name = "DISCARD",
  .summary = "Discards the current started transaction block.",
  .since = "0.2.0",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .flags.value = CMD_FLAG_WAITING_TX,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
