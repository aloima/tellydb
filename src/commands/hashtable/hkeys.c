#include <telly.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

static const uint64_t calculate_length(const struct HashTable *table) {
  uint64_t length = 0;

  for (uint32_t i = 0; i < table->size.allocated; ++i) {
    const struct HashTableField *field = table->fields[i];

    while (field) {
      const uint64_t line_length = (5 + get_digit_count(field->name.len) + field->name.len); // $number\r\nstring\r\n
      length += line_length;

      field = field->next;
    }
  }

  return length;
}


static void run(struct CommandEntry entry) {
  if (!entry.client) return;
  if (entry.data->arg_count != 1) {
    WRONG_ARGUMENT_ERROR(entry.client, "HKEYS");
    return;
  }

  const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

  if (!kv) {
    _write(entry.client, "*0\r\n", 4);
    return;
  }

  if (kv->type != TELLY_HASHTABLE) {
    INVALID_TYPE_ERROR(entry.client, "HKEYS");
    return;
  }

  const struct HashTable *table = kv->value;
  const uint64_t length = calculate_length(table);
  char *response = malloc(length);

  response[0] = '*';
  uint64_t at = ltoa(table->size.all, response + 1) + 1;
  response[at++] = '\r';
  response[at++] = '\n';

  for (uint32_t i = 0; i < table->size.allocated; ++i) {
    struct HashTableField *field = table->fields[i];

    while (field) {
      response[at++] = '$';
      at += ltoa(field->name.len, response + at);
      response[at++] = '\r';
      response[at++] = '\n';

      memcpy(response + at, field->name.value, field->name.len);
      at += field->name.len;
      response[at++] = '\r';
      response[at++] = '\n';

      field = field->next;
    }
  }

  _write(entry.client, response, length);
  free(response);
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
