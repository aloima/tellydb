#include <telly.h>

static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count != 2) return;
  (void) insert_into_vector(server->keyspace, &entry->args->data[0]);
}



static string_t run(struct CommandEntry *entry) {
  if (entry->args->count != 1) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("LPOP");
  }

  const string_t key = entry->args->data[0];
  const struct KVPair *kv = get_data(entry->database, key);

  if (!kv) {
    PASS_NO_CLIENT(entry->client);
    return RESP_NULL(entry->client->protover);
  }

  if (kv->type != TELLY_LIST) {
    PASS_NO_CLIENT(entry->client);
    return INVALID_TYPE_ERROR("LPOP");
  }

  LinkedList *list = kv->value;
  string_t response = EMPTY_STRING();

  if (entry->client) {
    DatabaseListNode *value = (DatabaseListNode *) list->begin->data;
    response = write_value(value->data, value->type, entry->client->protover, entry->client->write_buf);
  }

  if (list->size == 1) {
    delete_data(entry->database, key);
  } else {
    // Guaranteed that list exists and its size is least 1
    ASSERT(ll_remove_front(list, free_databaselistnode), ==, true);
  }

  return response;
}

const struct Command cmd_lpop = {
  .name = "LPOP",
  .summary = "Removes and returns first element of the list.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .permissions = (P_READ | P_WRITE),
  .flags.value = (CMD_FLAG_ACCESS_DATABASE | CMD_FLAG_AFFECT_DATABASE),
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
