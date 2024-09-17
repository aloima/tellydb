#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (client && (data->count == 2 || data->count % 2 != 0)) {
    write(client->connfd, "-Wrong argument count for 'HSET' command\r\n", 42);
    return;
  }

  string_t key = data->value.array[1]->value.string;
  struct KVPair *pair = get_data(key.value, conf);
  struct HashTable *table;

  if (pair && pair->type == TELLY_HASHTABLE) {
    table = pair->value.hashtable;
  } else {
    table = create_hashtable(32, 0.6);

    set_data((struct KVPair) {
      .key = key,
      .value = {
        .hashtable = table
      },
      .type = TELLY_HASHTABLE
    }, conf);
  }

  const uint64_t fv_count = (data->count / 2) - 1;

  for (uint32_t i = 1; i <= fv_count; ++i) {
    char *name = data->value.array[i * 2]->value.string.value;
    char *value = data->value.array[i * 2 + 1]->value.string.value;

    bool is_true = streq(value, "true");

    if (is_integer(value)) {
      int value_as_int = atoi(value);
      set_fv_of_hashtable(table, name, &value_as_int, TELLY_INT);
    } else if (is_true || streq(value, "false")) {
      set_fv_of_hashtable(table, name, &is_true, TELLY_BOOL);
    } else if (streq(value, "null")) {
      set_fv_of_hashtable(table, name, NULL, TELLY_NULL);
    } else {
      set_fv_of_hashtable(table, name, value, TELLY_STR);
    }
  }

  if (client) {
    const uint32_t buf_len = 3 + get_digit_count(fv_count);
    char buf[buf_len + 1];
    sprintf(buf, ":%ld\r\n", fv_count);

    write(client->connfd, buf, buf_len);
  }
}

struct Command cmd_hset = {
  .name = "HSET",
  .summary = "Sets field(s) of the hash table for the key. If hash table does not exist, creates it.",
  .since = "1.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
