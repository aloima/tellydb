#include <telly.h>

static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count != 1) return;
  ASSERT(insert_into_vector(server->keyspace, &entry->args->data[0]), ==, true);
}



static constexpr string_t c_null[] = {
  [RESP2] = CREATE_SIZED_STRING("$-1\r\n"),
  [RESP3] = CREATE_SIZED_STRING("_\r\n")
};

static constexpr string_t c_bool[4][2] = {
  [RESP2] = {
    [false] = RESP_OK_MESSAGE("false"),
    [true]  = RESP_OK_MESSAGE("true")
  },
  [RESP3] = {
    [false] = CREATE_SIZED_STRING("#f\r\n"),
    [true]  = CREATE_SIZED_STRING("#t\r\n")
  },
};

typedef struct Response {
  enum ProtocolVersion protover;
  char *data;
  uint64_t at;
} Response;

static void dump_hashtable(HashTableElement element, void *external) {
  const Value value = ((NameValue *) element.value)->value;

  Response *response = (Response *) external;
  const enum ProtocolVersion protover = response->protover;

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
    return WRONG_ARGUMENT_ERROR("HVALS");
  }

  const KeyValue *kv = get_data(entry->database, entry->args->data[0]);
  if (!kv)
    return CREATE_SIZED_STRING("*0\r\n");
  if (kv->value.type != TELLY_HASHTABLE)
    return INVALID_TYPE_ERROR("HVALS");

  HashTable *table = kv->value.data;
  const enum ProtocolVersion protover = entry->client->protover;

  char *buf = entry->client->write_buf;

  buf[0] = '*';
  uint64_t at = ltoa(table->size.count, buf + 1) + 1;
  buf[at++] = '\r';
  buf[at++] = '\n';

  Response response = { protover, buf, at };
  foreach_hashtable(table, dump_hashtable, &response);

  return CREATE_STRING(response.data, response.at);
}

const struct Command cmd_hvals = {
  .name = "HVALS",
  .summary = "Gets all field values from the hash table.",
  .since = "0.1.9",
  .complexity = "O(N) where N is hash table size",
  .permissions = P_READ,
  .flags.value = CMD_FLAG_ACCESS_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
