#include "../../../headers/telly.h"
#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/hashtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

static void run(struct Client *client, respdata_t *data) {
  if (client && (data->count == 2 || data->count % 2 != 0)) {
    WRONG_ARGUMENT_ERROR(client, "HSET", 4);
    return;
  }

  const string_t key = data->value.array[1]->value.string;
  struct KVPair *kv = get_data(key.value);
  struct HashTable *table;

  if (kv && kv->type == TELLY_HASHTABLE) {
    table = kv->value->hashtable;
  } else {
    table = create_hashtable(16, 0.5);
    set_data(kv, key, (value_t) {
      .hashtable = table
    }, TELLY_HASHTABLE);
  }

  const uint64_t fv_count = (data->count / 2) - 1;

  for (uint32_t i = 1; i <= fv_count; ++i) {
    char *name = data->value.array[i * 2]->value.string.value;
    char *value = data->value.array[i * 2 + 1]->value.string.value;

    bool is_true = streq(value, "true");

    if (is_integer(value)) {
      int value_as_int = atoi(value);
      add_fv_to_hashtable(table, name, &value_as_int, TELLY_INT);
    } else if (is_true || streq(value, "false")) {
      add_fv_to_hashtable(table, name, &is_true, TELLY_BOOL);
    } else if (streq(value, "null")) {
      add_fv_to_hashtable(table, name, NULL, TELLY_NULL);
    } else {
      add_fv_to_hashtable(table, name, value, TELLY_STR);
    }
  }

  if (client) {
    const uint32_t buf_len = 3 + get_digit_count(fv_count);
    char buf[buf_len + 1];
    sprintf(buf, ":%ld\r\n", fv_count);

    _write(client, buf, buf_len);
  }
}

struct Command cmd_hset = {
  .name = "HSET",
  .summary = "Sets field(s) of the hash table for the key. If hash table does not exist, creates it.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
