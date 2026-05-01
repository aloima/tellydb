#include <telly.h>

static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count != 1) return;
  (void) insert_into_vector(server->keyspace, &entry->args->data[0]);
}



static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 1) {
    return WRONG_ARGUMENT_ERROR("LLEN");
  }

  const struct KVPair *kv = get_data(entry->database, entry->args->data[0]);

  if (!kv) {
    return CREATE_STRING(":0\r\n", 4);
  } else if (kv->type != TELLY_LIST) {
    return INVALID_TYPE_ERROR("LLEN");
  }

  const size_t nbytes = create_resp_integer(entry->client->write_buf, ((LinkedList *) kv->value)->size);
  return CREATE_STRING(entry->client->write_buf, nbytes);
}

const struct Command cmd_llen = {
  .name = "LLEN",
  .summary = "Returns length of the list.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .permissions = P_READ,
  .flags.value = CMD_FLAG_ACCESS_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
