#include <telly.h>

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

static string_t run(struct CommandEntry entry) {
  PASS_NO_CLIENT(entry.client);

  if (entry.data->arg_count != 1) {
    return WRONG_ARGUMENT_ERROR("LLEN");
  }

  const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

  if (!kv) {
    return CREATE_STRING(":0\r\n", 4);
  } else if (kv->type != TELLY_LIST) {
    return INVALID_TYPE_ERROR("LLEN");
  }

  const size_t nbytes = sprintf(entry.buffer, ":%" PRIu32 "\r\n", ((struct List *) kv->value)->size);
  return CREATE_STRING(entry.buffer, nbytes);
}

const struct Command cmd_llen = {
  .name = "LLEN",
  .summary = "Returns length of the list.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .permissions = P_READ,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
