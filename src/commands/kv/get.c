#include <telly.h>

#include <stddef.h>

static string_t run(struct CommandEntry entry) {
  PASS_NO_CLIENT(entry.client);

  if (entry.data->arg_count != 1) {
    return WRONG_ARGUMENT_ERROR("GET");
  }

  const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

  if (kv) {
    write_value(entry.client, kv->value, kv->type);
  } else {
    return RESP_NULL(entry.client->protover);
  }

  PASS_COMMAND();
}

const struct Command cmd_get = {
  .name = "GET",
  .summary = "Gets value.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .permissions = P_READ,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
