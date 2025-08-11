#include <telly.h>

#include <stddef.h>

static void run(struct CommandEntry entry) {
  if (!entry.client) return;
  if (entry.data->arg_count != 2) {
    WRONG_ARGUMENT_ERROR(entry.client, "HGET");
    return;
  }

  const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

  if (!kv) {
    WRITE_NULL_REPLY(entry.client);
    return;
  }

  if (kv->type != TELLY_HASHTABLE) {
    INVALID_TYPE_ERROR(entry.client, "HGET");
    return;
  }

  const struct HashTableField *field = get_field_from_hashtable(kv->value, entry.data->args[1]);

  if (field) {
    write_value(entry.client, field->value, field->type);
  } else {
    WRITE_NULL_REPLY(entry.client);
  }
}

const struct Command cmd_hget = {
  .name = "HGET",
  .summary = "Gets a field from the hash table.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .permissions = P_READ,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
