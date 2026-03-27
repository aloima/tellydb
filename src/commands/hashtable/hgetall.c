#include <telly.h>

static constexpr string_t c_null[] = {
  [RESP2] = CREATE_STRING("$-1\r\n", 5),
  [RESP3] = CREATE_STRING("_\r\n", 3)
};

static constexpr string_t c_bool[4][2] = {
  [RESP2] = { [false] = CREATE_STRING("+false\r\n", 8), [true] = CREATE_STRING("+true\r\n", 7) },
  [RESP3] = { [false] = CREATE_STRING("#f\r\n", 4), [true] = CREATE_STRING("#t\r\n", 4) },
};

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 1) {
    return WRONG_ARGUMENT_ERROR("HGETALL");
  }

  const struct KVPair *kv = get_data(entry->database, entry->args->data[0]);

  if (!kv) {
    switch (entry->client->protover) {
      case RESP2:
        return CREATE_STRING("*0\r\n", 4);

      case RESP3:
        return CREATE_STRING("%0\r\n", 4);
    }
  }

  if (kv->type != TELLY_HASHTABLE) {
    return INVALID_TYPE_ERROR("HGETALL");
  }

  const struct HashTable *table = kv->value;
  const enum ProtocolVersion protover = entry->client->protover;

  char *response = entry->client->write_buf;
  uint64_t at;

  switch (protover) {
    case RESP2:
      response[0] = '*';
      at = ltoa(table->size.used * 2, response + 1) + 1;
      break;

    case RESP3:
      response[0] = '%';
      at = ltoa(table->size.used, response + 1) + 1;
      break;
  }

  response[at++] = '\r';
  response[at++] = '\n';

  for (uint32_t i = 0; i < table->size.capacity; ++i) {
    struct HashTableField *field = table->fields[i];
    if (!field) continue;

    at += create_resp_string(response + at, field->name);
    char *buf = response + at;

    switch (field->type) {
      case TELLY_NULL: {
        const string_t constant = c_null[protover];
        memcpy(buf, constant.value, constant.len);
        at += constant.len;
        break;
      }

      case TELLY_INT:
        at += create_resp_integer_mpz(protover, buf, *((mpz_t *) field->value));
        break;

      case TELLY_DOUBLE:
        at += create_resp_integer_mpf(protover, buf, *((mpf_t *) field->value));
        break;

      case TELLY_STR:
        at += create_resp_string(buf, *((string_t *) field->value));
        break;

      case TELLY_BOOL: {
        string_t constant = c_bool[protover][*((bool *) field->value)];
        memcpy(buf, constant.value, constant.len);
        at += constant.len;
        break;
      }

      default:
        break;
    }
  }

  return CREATE_STRING(response, at);
}

const struct Command cmd_hgetall = {
  .name = "HGETALL",
  .summary = "Gets all fields and their values from the hash table.",
  .since = "0.1.9",
  .complexity = "O(N) where N is hash table size",
  .permissions = P_READ,
  .flags.value = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
