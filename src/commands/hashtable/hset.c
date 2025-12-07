#include <telly.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <gmp.h>

static string_t run(struct CommandEntry *entry) {
  if (entry->data->arg_count == 1 || (entry->data->arg_count - 1) % 2 != 0) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("HSET");
  }

  const string_t key = entry->data->args[0];
  struct KVPair *kv = get_data(entry->database, key);
  struct HashTable *table;

  if (kv) {
    if (kv->type == TELLY_HASHTABLE) {
      table = kv->value;
    } else {
      PASS_NO_CLIENT(entry->client);
      return INVALID_TYPE_ERROR("HSET");
    }
  } else {
    table = create_hashtable(16);
    set_data(entry->database, kv, key, table, TELLY_HASHTABLE);
  }

  const uint32_t fv_count = (entry->data->arg_count - 1) / 2;

  for (uint32_t i = 1; i <= fv_count; ++i) {
    const string_t name = entry->data->args[(i * 2) - 1];
    const string_t input = entry->data->args[i * 2];

    const bool is_true = streq(input.value, "true");
    const bool is_integer = try_parse_integer(input.value);
    const bool is_double = try_parse_double(input.value);

    if (is_integer || is_double) {
      if (is_integer) {
        mpz_t *value = malloc(sizeof(mpz_t));
        mpz_init_set_str(*value, input.value, 10);

        set_field_of_hashtable(table, name, value, TELLY_INT);
      } else if (is_double) {
        mpf_t *value = malloc(sizeof(mpf_t));
        mpf_init2(*value, FLOAT_PRECISION);
        mpf_set_str(*value, input.value, 10);

        set_field_of_hashtable(table, name, value, TELLY_DOUBLE);
      }
    } else if (is_true || streq(input.value, "false")) {
      bool *value = malloc(sizeof(bool));
      *value = is_true;

      set_field_of_hashtable(table, name, value, TELLY_BOOL);
    } else if (streq(input.value, "null")) {
      set_field_of_hashtable(table, name, NULL, TELLY_NULL);
    } else {
      string_t *value = malloc(sizeof(string_t));
      value->len = input.len;
      value->value = malloc(value->len);
      memcpy(value->value, input.value, value->len);

      set_field_of_hashtable(table, name, value, TELLY_STR);
    }
  }

  PASS_NO_CLIENT(entry->client)
  const size_t buf_len = create_resp_integer(entry->client->write_buf, fv_count);
  return CREATE_STRING(entry->client->write_buf, buf_len);
}

const struct Command cmd_hset = {
  .name = "HSET",
  .summary = "Sets field(s) of the hash table.",
  .since = "0.1.3",
  .complexity = "O(N) where N is written field name-value pair count",
  .permissions = P_WRITE,
  .flags = CMD_FLAG_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
