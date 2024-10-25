#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/hashtable.h"
#include "../../../headers/utils.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

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
      table = kv->value;
    } else {
      if (client) _write(client, "-Invalid type for 'HDEL' command\r\n", 34);
      return;
    }
  } else {
    if (client) _write(client, ":0\r\n", 4);
    return;
  }

  if (client) {
    const uint32_t old_size = table->size.all;

    for (uint32_t i = 2; i < data->count; ++i) {
      del_fv_to_hashtable(table, data->value.array[i]->value.string);
    }

    char buf[14];
    const size_t nbytes = sprintf(buf, ":%d\r\n", old_size - table->size.all);
    _write(client, buf, nbytes);
  } else {
    for (uint32_t i = 2; i < data->count; ++i) {
      del_fv_to_hashtable(table, data->value.array[i]->value.string);
    }
  }
}

struct Command cmd_hdel = {
  .name = "HDEL",
  .summary = "Deletes field(s) of the hash table for the key. If hash table does not exist, creates it.",
  .since = "0.1.5",
  .complexity = "O(N) where N is written field name count",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
