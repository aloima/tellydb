#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (command->arg_count < 2) {
    if (client) WRONG_ARGUMENT_ERROR(client, "HDEL", 4);
    return;
  }

  if (password->permissions & P_WRITE) {
    const string_t key = command->args[0];
    struct KVPair *kv = get_data(key);
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
      if (password->permissions & P_READ) {
        const uint32_t old_size = table->size.all;

        for (uint32_t i = 1; i < command->arg_count; ++i) {
          del_fv_to_hashtable(table, command->args[i]);
        }

        if (table->size.all == 0) delete_data(key);

        char buf[14];
        const size_t nbytes = sprintf(buf, ":%d\r\n", old_size - table->size.all);
        _write(client, buf, nbytes);
      } else {
        _write(client, "-Not allowed to use this command, need P_READ\r\n", 47);
      }
    } else {
      for (uint32_t i = 1; i < command->arg_count; ++i) {
        del_fv_to_hashtable(table, command->args[i]);
      }

      if (table->size.all == 0) delete_data(key);
    }
  } else if (client) {
    _write(client, "-Not allowed to use this command, need P_WRITE\r\n", 48);
  }
}

const struct Command cmd_hdel = {
  .name = "HDEL",
  .summary = "Deletes field(s) of the hash table.",
  .since = "0.1.5",
  .complexity = "O(N) where N is written field name count",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
