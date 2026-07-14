#include <telly.h>

static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count != 2) return;
  ASSERT(insert_into_vector(server->keyspace, &entry->args->data[0]), ==, true);
}



static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 2) {
    return WRONG_ARGUMENT_ERROR("HTYPE");
  }

  const KeyValue *kv = get_data(entry->database, entry->args->data[0]);
  if (!kv)
    return RESP_NULL(entry->client->protover);
  if (kv->value.type != TELLY_HASHTABLE)
    return INVALID_TYPE_ERROR("HTYPE");

  HashTable *table = (HashTable *) kv->value.data;
  string_t *key = &entry->args->data[1];

  const HashTableNameValue *field = (HashTableNameValue *) get_from_hashtable(table, key);
  if (field == NULL)
    return RESP_NULL(entry->client->protover);

  const Value value = field->value->value;
  return get_resp_type_name(value.type);
}

const struct Command cmd_htype = {
  .name = "HTYPE",
  .summary = "Returns type of the field from hash table.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .permissions = P_READ,
  .flags.value = CMD_FLAG_ACCESS_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
