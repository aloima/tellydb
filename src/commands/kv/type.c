#include <telly.h>

#include <stddef.h>

static string_t run(struct CommandEntry entry) {
  PASS_NO_CLIENT(entry.client);

  if (entry.data->arg_count != 1) {
    return WRONG_ARGUMENT_ERROR("TYPE");
  }

  struct KVPair *res = get_data(entry.database, entry.data->args[0]);

  if (!res) {
    return RESP_NULL(entry.client->protover);
  }

  switch (res->type) {
    case TELLY_NULL:
      return RESP_OK_MESSAGE("null");

    case TELLY_INT:
      return RESP_OK_MESSAGE("integer");

    case TELLY_DOUBLE:
      return RESP_OK_MESSAGE("double");

    case TELLY_STR:
      return RESP_OK_MESSAGE("string");

    case TELLY_HASHTABLE:
      return RESP_OK_MESSAGE("hash table");

    case TELLY_LIST:
      return RESP_OK_MESSAGE("list");

    case TELLY_BOOL:
      return RESP_OK_MESSAGE("boolean");

    default:
      PASS_COMMAND();
  }
}

const struct Command cmd_type = {
  .name = "TYPE",
  .summary = "Returns type of the value.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
