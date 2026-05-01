#include <telly.h>

static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count < 2) return;
  (void) insert_into_vector(server->keyspace, &entry->args->data[0]);
}



#define LPUSH(list, node, value, _type) do {    \
  (node)->type = (_type);                       \
  (node)->data = (value);                       \
  if (ll_insert_front((list), (node)) == NULL)  \
    return RESP_ERROR_MESSAGE("Out of memory"); \
} while (0)

static string_t run(struct CommandEntry *entry) {
  if (entry->args->count < 2) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("LPUSH");
  }

  const string_t key = entry->args->data[0];
  struct KVPair *kv = get_data(entry->database, key);
  struct LinkedList *list;

  if (kv) {
    if (kv->type != TELLY_LIST) {
      PASS_NO_CLIENT(entry->client);
      return INVALID_TYPE_ERROR("LPUSH");
    }

    list = kv->value;
  } else {
    list = ll_create();
    if (list == NULL) return RESP_ERROR_MESSAGE("Out of memory");
    set_data(entry->database, kv, key, list, TELLY_LIST, NULL);
  }

  for (uint32_t i = 1; i < entry->args->count; ++i) {
    DatabaseListNode *node = malloc(sizeof(DatabaseListNode));
    if (node == NULL) return RESP_ERROR_MESSAGE("Out of memory");

    const string_t input = entry->args->data[i];
    bool is_true = streq(input.value, "true");

    if (try_parse_integer(input.value)) {
      mpz_t *value = malloc(sizeof(mpz_t));
      if (value == NULL) return RESP_ERROR_MESSAGE("Out of memory");
      mpz_init_set_str(*value, input.value, 10);

      LPUSH(list, node, value, TELLY_INT);
    } else if (try_parse_double(input.value)) {
      mpf_t *value = malloc(sizeof(mpf_t));
      if (value == NULL) return RESP_ERROR_MESSAGE("Out of memory");
      mpf_init2(*value, FLOAT_PRECISION);
      mpf_set_str(*value, input.value, 10);

      LPUSH(list, node, value, TELLY_DOUBLE);
    } else if (is_true || streq(input.value, "false")) {
      bool *value = malloc(sizeof(bool));
      if (value == NULL) return RESP_ERROR_MESSAGE("Out of memory");
      *value = is_true;

      LPUSH(list, node, value, TELLY_BOOL);
    } else if (streq(input.value, "null")) {
      LPUSH(list, node, NULL, TELLY_NULL);
    } else {
      string_t *value = malloc(sizeof(string_t));
      if (value == NULL) return RESP_ERROR_MESSAGE("Out of memory");

      const uint32_t size = input.len + 1;
      value->len = input.len;
      value->value = malloc(size);

      if (value->value == NULL) {
        free(value);
        return RESP_ERROR_MESSAGE("Out of memory");
      }

      memcpy(value->value, input.value, size);
      LPUSH(list, node, value, TELLY_STR);
    }
  }

  PASS_NO_CLIENT(entry->client);
  const size_t nbytes = create_resp_integer(entry->client->write_buf, entry->args->count - 1);
  return CREATE_STRING(entry->client->write_buf, nbytes);
}

#undef LPUSH

const struct Command cmd_lpush = {
  .name = "LPUSH",
  .summary = "Pushes element(s) to beginning of the list.",
  .since = "0.1.3",
  .complexity = "O(N) where N is written element count",
  .permissions = P_WRITE,
  .flags.value = (CMD_FLAG_ACCESS_DATABASE | CMD_FLAG_AFFECT_DATABASE),
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
