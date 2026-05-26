#include <telly.h>

static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count != 1) return;
  (void) insert_into_vector(server->keyspace, &entry->args->data[0]);
}



static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 1) {
    return WRONG_ARGUMENT_ERROR("HLEN");
  }

  const KeyValue *kv = get_data(entry->database, entry->args->data[0]);
  if (!kv)
    return RESP_NULL(entry->client->protover);
  if (kv->value.type != TELLY_HASHTABLE)
    return INVALID_TYPE_ERROR("HLEN");

  const HashTable *table = kv->value.data;

  const size_t nbytes = sprintf(entry->client->write_buf, (
    "*2\r\n"
      ":%lu\r\n"
      ":%lu\r\n"
  ), table->size.count, table->size.capacity);

  return CREATE_STRING(entry->client->write_buf, nbytes);
}

const struct Command cmd_hlen = {
  .name = "HLEN",
  .summary = "Returns field count information of the hash table.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .permissions = P_READ,
  .flags.value = CMD_FLAG_ACCESS_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
