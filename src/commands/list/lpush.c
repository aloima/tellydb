#include <telly.h>

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <gmp.h>

static void inline lpush_to_list(struct List *list, void *value, enum TellyTypes type) {
  struct ListNode *node = create_listnode(value, type);
  node->next = list->begin;
  list->begin = node;
  list->size += 1;

  if (list->size == 1) {
    list->end = node;
  } else {
    node->next->prev = node;
  }
}

static string_t run(struct CommandEntry *entry) {
  if (entry->args->count < 2) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("LPUSH");
  }

  const string_t key = entry->args->data[0];
  struct KVPair *kv = get_data(entry->database, key);
  struct List *list;

  if (kv) {
    if (kv->type != TELLY_LIST) {
      PASS_NO_CLIENT(entry->client);
      return INVALID_TYPE_ERROR("LPUSH");
    }

    list = kv->value;
  } else {
    list = create_list();
    if (list == NULL) return RESP_ERROR_MESSAGE("Out of memory");
    set_data(entry->database, kv, key, list, TELLY_LIST);
  }

  for (uint32_t i = 1; i < entry->args->count; ++i) {
    string_t input = entry->args->data[i];
    bool is_true = streq(input.value, "true");

    if (try_parse_integer(input.value)) {
      mpz_t *value = malloc(sizeof(mpz_t));
      if (value == NULL) return RESP_ERROR_MESSAGE("Out of memory");
      mpz_init_set_str(*value, input.value, 10);

      lpush_to_list(list, value, TELLY_INT);
    } else if (try_parse_double(input.value)) {
      mpf_t *value = malloc(sizeof(mpf_t));
      if (value == NULL) return RESP_ERROR_MESSAGE("Out of memory");
      mpf_init2(*value, FLOAT_PRECISION);
      mpf_set_str(*value, input.value, 10);

      lpush_to_list(list, value, TELLY_DOUBLE);
    } else if (is_true || streq(input.value, "false")) {
      bool *value = malloc(sizeof(bool));
      if (value == NULL) return RESP_ERROR_MESSAGE("Out of memory");
      *value = is_true;

      lpush_to_list(list, value, TELLY_BOOL);
    } else if (streq(input.value, "null")) {
      lpush_to_list(list, NULL, TELLY_NULL);
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

      lpush_to_list(list, value, TELLY_STR);
    }
  }

  PASS_NO_CLIENT(entry->client);
  const size_t nbytes = create_resp_integer(entry->client->write_buf, entry->args->count - 1);
  return CREATE_STRING(entry->client->write_buf, nbytes);
}

const struct Command cmd_lpush = {
  .name = "LPUSH",
  .summary = "Pushes element(s) to beginning of the list.",
  .since = "0.1.3",
  .complexity = "O(N) where N is written element count",
  .permissions = P_WRITE,
  .flags.value = CMD_FLAG_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
