#include <telly.h>

#include <stddef.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->data->arg_count != 2) {
    return WRONG_ARGUMENT_ERROR("HGET");
  }

  const struct KVPair *kv = get_data(entry->database, entry->data->args[0]);

  if (!kv) {
    return RESP_NULL(entry->client->protover);
  }

  if (kv->type != TELLY_HASHTABLE) {
    return INVALID_TYPE_ERROR("HGET");
  }

  const struct HashTableField *field = get_field_from_hashtable(kv->value, entry->data->args[1]);

  if (field) {
    return write_value(field->value, field->type, entry->client->protover, entry->buffer);
  } else {
    return RESP_NULL(entry->client->protover);
  }
}

const struct Command cmd_hget = {
  .name = "HGET",
  .summary = "Gets a field from the hash table.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .permissions = P_READ,
  .flags = CMD_FLAG_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
