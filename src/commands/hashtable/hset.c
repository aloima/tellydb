#include <telly.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

static void run(struct CommandEntry entry) {
  if (entry.data->arg_count == 1 || (entry.data->arg_count - 1) % 2 != 0) {
    if (entry.client) {
      WRONG_ARGUMENT_ERROR(entry.client, "HSET");
    }

    return;
  }

  const string_t key = entry.data->args[0];
  struct KVPair *kv = get_data(entry.database, key);
  struct HashTable *table;

  if (kv) {
    if (kv->type == TELLY_HASHTABLE) {
      table = kv->value;
    } else {
      if (entry.client) {
        INVALID_TYPE_ERROR(entry.client, "HSET");
      }

      return;
    }
  } else {
    table = create_hashtable(16);
    set_data(entry.database, kv, key, table, TELLY_HASHTABLE);
  }

  const uint32_t fv_count = (entry.data->arg_count - 1) / 2;

  for (uint32_t i = 1; i <= fv_count; ++i) {
    const string_t name = entry.data->args[(i * 2) - 1];
    const string_t input = entry.data->args[i * 2];
    char *input_value = input.value;

    bool is_true = streq(input_value, "true");

    if (is_integer(input_value)) {
      const long number = atol(input.value);
      long *value = malloc(sizeof(long));
      *value = number;

      set_field_of_hashtable(table, name, value, TELLY_NUM);
    } else if (is_true || streq(input_value, "false")) {
      bool *value = malloc(sizeof(bool));
      *value = is_true;

      set_field_of_hashtable(table, name, value, TELLY_BOOL);
    } else if (streq(input_value, "null")) {
      set_field_of_hashtable(table, name, NULL, TELLY_NULL);
    } else {
      string_t *value = malloc(sizeof(string_t));
      value->len = input.len;
      value->value = malloc(value->len);
      memcpy(value->value, input_value, value->len);

      set_field_of_hashtable(table, name, value, TELLY_STR);
    }
  }

  if (entry.client) {
    char buf[14];
    const size_t buf_len = create_resp_integer(buf, fv_count);

    _write(entry.client, buf, buf_len);
  }
}

const struct Command cmd_hset = {
  .name = "HSET",
  .summary = "Sets field(s) of the hash table.",
  .since = "0.1.3",
  .complexity = "O(N) where N is written field name-value pair count",
  .permissions = P_WRITE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
