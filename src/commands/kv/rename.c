#include <telly.h>

// Runs before run(), so old key must be used.
static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count != 2) return;

  (void) insert_into_vector(server->keyspace, &entry->args->data[0]);
}



static string_t run(struct CommandEntry *entry) {
  if (entry->args->count != 2) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("RENAME");
  }

  string_t search = entry->args->data[0];
  const string_t name = entry->args->data[1];

  KeyValue *kv = get_data(entry->database, search);
  if (kv == NULL) {
    PASS_NO_CLIENT(entry->client);
    return RESP_ERROR_MESSAGE("The new key already exists");
  }

  (void) delete_from_hashtable(entry->database->data, &search);

  free(kv->key.value);
  kv->key.len = name.len;
  kv->key.value = malloc(name.len + 1);
  memcpy(kv->key.value, name.value, name.len);

  (void) insert_into_hashtable(entry->database->data, &kv->key, kv);

  PASS_NO_CLIENT(entry->client);
  return RESP_OK();
}

const struct Command cmd_rename = {
  .name = "RENAME",
  .summary = "Renames existing key to new key.",
  .since = "0.1.7",
  .complexity = "O(1)",
  .permissions = P_WRITE,
  .flags.value = (CMD_FLAG_ACCESS_DATABASE | CMD_FLAG_AFFECT_DATABASE),
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
