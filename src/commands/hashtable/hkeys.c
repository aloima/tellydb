#include <telly.h>

#include <string.h>
#include <stdint.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->data->arg_count != 1) {
    return WRONG_ARGUMENT_ERROR("HKEYS");
  }

  const struct KVPair *kv = get_data(entry->database, entry->data->args[0]);

  if (!kv) {
    return CREATE_STRING("*0\r\n", 4);
  }

  if (kv->type != TELLY_HASHTABLE) {
    return INVALID_TYPE_ERROR("HKEYS");
  }

  const struct HashTable *table = kv->value;
  char *response = entry->buffer;

  response[0] = '*';
  uint64_t at = ltoa(table->size.used, response + 1) + 1;
  response[at++] = '\r';
  response[at++] = '\n';

  for (uint32_t i = 0; i < table->size.capacity; ++i) {
    struct HashTableField *field = table->fields[i];

    if (field) {
      response[at++] = '$';
      at += ltoa(field->name.len, response + at);
      response[at++] = '\r';
      response[at++] = '\n';

      memcpy(response + at, field->name.value, field->name.len);
      at += field->name.len;
      response[at++] = '\r';
      response[at++] = '\n';
    }
  }

  return CREATE_STRING(response, at);
}

const struct Command cmd_hkeys = {
  .name = "HKEYS",
  .summary = "Gets all field names from the hash table.",
  .since = "0.1.9",
  .complexity = "O(N) where N is hash table size",
  .permissions = P_READ,
  .flags = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
