#include <telly.h>

static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count != 1) return;
  (void) insert_into_vector(server->keyspace, &entry->args->data[0]);
}



typedef struct Response {
  char *buf;
  uint64_t at;
} Response;

static void dump_hashtable_keys(HashTableElement element, void *external) {
  const string_t *name = (string_t *) element.key;
  Response *response = (Response *) external;

  char *buf = response->buf;
  uint64_t at = response->at;

  buf[at++] = '$';
  at += ltoa(name->len, buf + at);
  buf[at++] = '\r';
  buf[at++] = '\n';

  memcpy(buf + at, name->value, name->len);
  at += name->len;
  buf[at++] = '\r';
  buf[at++] = '\n';

  response->at = at;
}

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 1) {
    return WRONG_ARGUMENT_ERROR("HKEYS");
  }

  const KeyValue *kv = get_data(entry->database, entry->args->data[0]);
  if (!kv)
    return CREATE_STRING("*0\r\n", 4);
  if (kv->value.type != TELLY_HASHTABLE)
    return INVALID_TYPE_ERROR("HKEYS");

  HashTable *table = (HashTable *) kv->value.data;
  char *buf = entry->client->write_buf;

  buf[0] = '*';
  uint64_t at = ltoa(table->size.count, buf + 1) + 1;
  buf[at++] = '\r';
  buf[at++] = '\n';

  Response response = {buf, at};
  foreach_hashtable(table, dump_hashtable_keys, &response);

  // `at` has old value, we copied `at` into `response`. So, we need to use that.
  return CREATE_STRING(response.buf, response.at);
}

const struct Command cmd_hkeys = {
  .name = "HKEYS",
  .summary = "Gets all field names from the hash table.",
  .since = "0.1.9",
  .complexity = "O(N) where N is hash table size",
  .permissions = P_READ,
  .flags.value = CMD_FLAG_ACCESS_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
