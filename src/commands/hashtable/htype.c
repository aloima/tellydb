#include <telly.h>

#include <stddef.h>

static string_t run(struct CommandEntry entry) {
  PASS_NO_CLIENT(entry.client);

  if (entry.data->arg_count != 2) {
    return WRONG_ARGUMENT_ERROR("HTYPE");
  }

  const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

  if (!kv) {
    return RESP_NULL(entry.client->protover);
  }

  if (kv->type != TELLY_HASHTABLE) {
    return INVALID_TYPE_ERROR("HTYPE");
  }

  const struct HashTableField *field = get_field_from_hashtable(kv->value, entry.data->args[1]);

  switch (field->type) {
    case TELLY_NULL:
      return RESP_OK_MESSAGE("null");

    case TELLY_NUM:
      return RESP_OK_MESSAGE("number");

    case TELLY_STR:
      return RESP_OK_MESSAGE("string");

    case TELLY_BOOL:
      return RESP_OK_MESSAGE("boolean");

    default:
      PASS_COMMAND();
  }
}

const struct Command cmd_htype = {
  .name = "HTYPE",
  .summary = "Returns type of the field from hash table.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .permissions = P_READ,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
