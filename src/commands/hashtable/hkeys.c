#include <telly.h>

#include <string.h>
#include <stdint.h>

static const uint64_t calculate_length(const struct HashTable *table) {
  uint64_t length = (3 + get_digit_count(table->size.used)); // *number\r\n

  for (uint32_t i = 0; i < table->size.capacity; ++i) {
    const struct HashTableField *field = table->fields[i];

    if (field) {
      const uint64_t line_length = (5 + get_digit_count(field->name.len) + field->name.len); // $number\r\nstring\r\n
      length += line_length;
    }
  }

  return length;
}

static string_t run(struct CommandEntry entry) {
  PASS_NO_CLIENT(entry.client);

  if (entry.data->arg_count != 1) {
    return WRONG_ARGUMENT_ERROR("HKEYS");
  }

  const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

  if (!kv) {
    return CREATE_STRING("*0\r\n", 4);
  }

  if (kv->type != TELLY_HASHTABLE) {
    return INVALID_TYPE_ERROR("HKEYS");
  }

  const struct HashTable *table = kv->value;
  const uint64_t length = calculate_length(table);
  char *response = entry.buffer;

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

  return CREATE_STRING(response, length);
}

const struct Command cmd_hkeys = {
  .name = "HKEYS",
  .summary = "Gets all field names from the hash table.",
  .since = "0.1.9",
  .complexity = "O(N) where N is hash table size",
  .permissions = P_READ,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
