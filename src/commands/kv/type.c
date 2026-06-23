#include <telly.h>

static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count != 1) return;

  ASSERT(insert_into_vector(server->keyspace, &entry->args->data[0]), ==, true);
}



static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 1)
    return WRONG_ARGUMENT_ERROR("TYPE");

  KeyValue *res = get_data(entry->database, entry->args->data[0]);

  if (!res)
    return RESP_NULL(entry->client->protover);

  return get_resp_type_name(res->value.type);
}

const struct Command cmd_type = {
  .name = "TYPE",
  .summary = "Returns type of the value.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .flags.value = CMD_FLAG_ACCESS_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
