#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/hashtable.h"
#include "../../../headers/utils.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

static void run(struct Client *client, respdata_t *data) {
  if (data->count == 2 || data->count % 2 != 0) {
    if (client) WRONG_ARGUMENT_ERROR(client, "HSET", 4);
    return;
  }

  const string_t key = data->value.array[1]->value.string;
  struct KVPair *kv = get_data(key.value);
  struct HashTable *table;

  if (kv && kv->type == TELLY_HASHTABLE) {
    table = kv->value;
  } else {
    table = create_hashtable(16, 0.5, 0.75);
    set_data(kv, key, table, TELLY_HASHTABLE);
  }

  const uint32_t fv_count = (data->count / 2) - 1;

  for (uint32_t i = 1; i <= fv_count; ++i) {
    const string_t name = data->value.array[i * 2]->value.string;
    const string_t input = data->value.array[i * 2 + 1]->value.string;
    char *input_value = input.value;

    bool is_true = streq(input_value, "true");

    if (is_integer(input_value)) {
      const long number = atol(input.value);
      long *value = malloc(sizeof(long));
      memcpy(value, &number, sizeof(long));

      add_fv_to_hashtable(table, name, value, TELLY_NUM);
    } else if (is_true || streq(input_value, "false")) {
      bool *value = malloc(sizeof(bool));
      memset(value, is_true, sizeof(bool));

      add_fv_to_hashtable(table, name, value, TELLY_BOOL);
    } else if (streq(input_value, "null")) {
      add_fv_to_hashtable(table, name, NULL, TELLY_NULL);
    } else {
      string_t *value = malloc(sizeof(string_t));
      const uint32_t size = input.len + 1;
      value->len = input.len;
      value->value = malloc(size);
      memcpy(value->value, input_value, size);

      add_fv_to_hashtable(table, name, value, TELLY_STR);
    }
  }

  if (client) {
    char buf[14];
    const size_t buf_len = sprintf(buf, ":%d\r\n", fv_count);

    _write(client, buf, buf_len);
  }
}

struct Command cmd_hset = {
  .name = "HSET",
  .summary = "Sets field(s) of the hash table for the key. If hash table does not exist, creates it.",
  .since = "0.1.3",
  .complexity = "O(N) where N is field name-value pair count",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
