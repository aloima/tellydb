#include <telly.h>

static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count < 2) return;
  ASSERT(insert_into_vector(server->keyspace, &entry->args->data[0]), ==, true);
}



static string_t run(struct CommandEntry *entry) {
  if (entry->args->count < 2) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("HDEL");
  }

  const string_t key = entry->args->data[0];
  KeyValue *kv = get_data(entry->database, key);
  HashTable *table;

  if (kv) {
    if (kv->value.type == TELLY_HASHTABLE) {
      table = kv->value.data;
    } else {
      PASS_NO_CLIENT(entry->client);
      return INVALID_TYPE_ERROR("HDEL");
    }
  } else {
    PASS_NO_CLIENT(entry->client);
    return CREATE_SIZED_STRING(":0\r\n");
  }

  if (!(entry->password->permissions & P_READ)) {
    if (entry->client)
      return RESP_ERROR_MESSAGE("Not allowed to use this command, need P_READ");
    else
      PASS_COMMAND();
  }

  const uint32_t old_size = table->size.count;

  for (uint32_t i = 1; i < entry->args->count; ++i) {
    delete_from_hashtable(table, &entry->args->data[i]);
  }

  const uint32_t current_size = table->size.count;

  if (table->size.count == 0) {
    delete_data(entry->database, key);
  }

  if (entry->client) {
    const size_t nbytes = create_resp_integer(entry->client->write_buf, old_size - current_size);
    return CREATE_STRING(entry->client->write_buf, nbytes);
  } else
    PASS_COMMAND();
}

const struct Command cmd_hdel = {
  .name = "HDEL",
  .summary = "Deletes field(s) of the hash table.",
  .since = "0.1.5",
  .complexity = "O(N) where N is written field name count",
  .permissions = P_WRITE,
  .flags.value = (CMD_FLAG_ACCESS_DATABASE | CMD_FLAG_AFFECT_DATABASE),
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
