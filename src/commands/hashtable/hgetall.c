#include <telly.h>

static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count != 1) return;
  (void) insert_into_vector(server->keyspace, &entry->args->data[0]);
}



static constexpr string_t c_null[] = {
  [RESP2] = CREATE_STRING("$-1\r\n", 5),
  [RESP3] = CREATE_STRING("_\r\n", 3)
};

static constexpr string_t c_bool[4][2] = {
  [RESP2] = { [false] = CREATE_STRING("+false\r\n", 8), [true] = CREATE_STRING("+true\r\n", 7) },
  [RESP3] = { [false] = CREATE_STRING("#f\r\n", 4), [true] = CREATE_STRING("#t\r\n", 4) },
};

typedef struct Response {
  enum ProtocolVersion protover;
  char *data;
  uint64_t at;
} Response;

static void dump_hashtable(HashTableElement element, void *external) {
  const string_t *name = (string_t *) element.key;
  const Value value = ((NameValue *) element.value)->value;

  Response *response = (Response *) external;
  const enum ProtocolVersion protover = response->protover;

  response->at += create_resp_string(response->data + response->at, *name);
  char *buf = response->data + response->at;

  switch (value.type) {
    case TELLY_NULL: {
      const string_t constant = c_null[protover];
      memcpy(buf, constant.value, constant.len);
      response->at += constant.len;
      break;
    }

    case TELLY_INT:
      response->at += create_resp_integer_mpz(protover, buf, *((mpz_t *) value.data));
      break;

    case TELLY_DOUBLE:
      response->at += create_resp_integer_mpf(protover, buf, *((mpf_t *) value.data));
      break;

    case TELLY_STR:
      response->at += create_resp_string(buf, *((string_t *) value.data));
      break;

    case TELLY_BOOL: {
      string_t constant = c_bool[protover][*((bool *) value.data)];
      memcpy(buf, constant.value, constant.len);
      response->at += constant.len;
      break;
    }

    default:
      break;
  }
}

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 1) {
    return WRONG_ARGUMENT_ERROR("HGETALL");
  }

  const KeyValue *kv = get_data(entry->database, entry->args->data[0]);

  if (!kv) {
    switch (entry->client->protover) {
      case RESP2:
        return CREATE_STRING("*0\r\n", 4);

      case RESP3:
        return CREATE_STRING("%0\r\n", 4);
    }
  }

  if (kv->value.type != TELLY_HASHTABLE) {
    return INVALID_TYPE_ERROR("HGETALL");
  }

  HashTable *table = (HashTable *) kv->value.data;
  const enum ProtocolVersion protover = entry->client->protover;

  char *data = entry->client->write_buf;
  uint64_t at;

  switch (protover) {
    case RESP2:
      data[0] = '*';
      at = ltoa(table->size.count * 2, data + 1) + 1;
      break;

    case RESP3:
      data[0] = '%';
      at = ltoa(table->size.count, data + 1) + 1;
      break;
  }

  data[at++] = '\r';
  data[at++] = '\n';

  Response response = { protover, data, at };
  foreach_hashtable(table, dump_hashtable, &response);

  // `at` has old value, we copied `at` into `response`. So, we need to use that.
  return CREATE_STRING(response.data, response.at);
}

const struct Command cmd_hgetall = {
  .name = "HGETALL",
  .summary = "Gets all fields and their values from the hash table.",
  .since = "0.1.9",
  .complexity = "O(N) where N is hash table size",
  .permissions = P_READ,
  .flags.value = CMD_FLAG_ACCESS_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
