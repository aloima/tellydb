#include "database/kv.h"
#include <telly.h>

static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count != 2) return;
  (void) insert_into_vector(server->keyspace, &entry->args->data[0]);
}



static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 2) {
    return WRONG_ARGUMENT_ERROR("HGET");
  }

  const KeyValue *kv = get_data(entry->database, entry->args->data[0]);
  if (!kv)
    return RESP_NULL(entry->client->protover);
  if (kv->value.type != TELLY_HASHTABLE)
    return INVALID_TYPE_ERROR("HGET");

  HashTable *table = (HashTable *) kv->value.data;
  string_t *name = &entry->args->data[1];

  const HashTableNameValue *field = (HashTableNameValue *) get_from_hashtable(table, name);
  const Value value = field->value->value;

  if (field) {
    return write_value(value.data, value.type, entry->client->protover, entry->client->write_buf);
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
  .flags.value = CMD_FLAG_ACCESS_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
