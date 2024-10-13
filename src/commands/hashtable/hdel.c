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
  if (data->count < 3) {
    if (client) WRONG_ARGUMENT_ERROR(client, "HDEL", 4);
    return;
  }

  const string_t key = data->value.array[1]->value.string;
  struct KVPair *kv = get_data(key.value);
  struct HashTable *table;

  if (kv) {
    if (kv->type == TELLY_HASHTABLE) {
      table = kv->value->hashtable;
    } else if (client) {
      _write(client, "-Invalid type for 'HDEL' command\r\n", 34);
      return;
    }
  } else if (client) {
    _write(client, ":0\r\n", 4);
    return;
  }

  uint32_t deleted = 0;

  for (uint32_t i = 1; i < data->count; ++i) {
    const string_t name = data->value.array[i]->value.string;

    if (del_fv_to_hashtable(table, name)) deleted += 1;
  }

  if (client) {
    const uint32_t buf_len = 3 + get_digit_count(deleted);
    char buf[buf_len + 1];
    sprintf(buf, ":%d\r\n", deleted);

    _write(client, buf, buf_len);
  }
}

struct Command cmd_hdel = {
  .name = "HDEL",
  .summary = "Deletes field(s) of the hash table for the key. If hash table does not exist, creates it.",
  .since = "0.1.5",
  .complexity = "O(N) where N is field name count",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
