#include <telly.h>

#include <stdio.h>
#include <stdint.h>

static string_t run(struct CommandEntry *entry) {
  if (entry->args->count < 2) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("HDEL");
  }

  const string_t key = entry->args->data[0];
  struct KVPair *kv = get_data(entry->database, key);
  struct HashTable *table;

  if (kv) {
    if (kv->type == TELLY_HASHTABLE) {
      table = kv->value;
    } else {
      PASS_NO_CLIENT(entry->client);
      return INVALID_TYPE_ERROR("HDEL");
    }
  } else {
    PASS_NO_CLIENT(entry->client);
    return CREATE_STRING(":0\r\n", 4);
  }

  if (!entry->client) {
    for (uint32_t i = 1; i < entry->args->count; ++i) {
      del_field_from_hashtable(table, entry->args->data[i]);
    }

    if (table->size.used == 0) {
      delete_data(entry->database, key);
    }

    PASS_COMMAND();
  }

  if (!(entry->password->permissions & P_READ)) {
    return RESP_ERROR_MESSAGE("Not allowed to use this command, need P_READ");
  }

  const uint32_t old_size = table->size.used;

  for (uint32_t i = 1; i < entry->args->count; ++i) {
    del_field_from_hashtable(table, entry->args->data[i]);
  }

  const uint32_t current_size = table->size.used;

  if (table->size.used == 0) {
    delete_data(entry->database, key);
  }

  const size_t nbytes = create_resp_integer(entry->client->write_buf, old_size - current_size);
  return CREATE_STRING(entry->client->write_buf, nbytes);
}

const struct Command cmd_hdel = {
  .name = "HDEL",
  .summary = "Deletes field(s) of the hash table.",
  .since = "0.1.5",
  .complexity = "O(N) where N is written field name count",
  .permissions = P_WRITE,
  .flags.value = CMD_FLAG_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
