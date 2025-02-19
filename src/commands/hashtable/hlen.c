#include "../../../headers/telly.h"

#include <stdio.h>

static void run(struct CommandEntry entry) {
  if (entry.client) {
    if (entry.data->arg_count != 1) {
      WRONG_ARGUMENT_ERROR(entry.client, "HLEN", 4);
      return;
    }

    if (entry.password->permissions & P_READ) {
      const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

      if (kv) {
        if (kv->type == TELLY_HASHTABLE) {
          const struct HashTable *table = kv->value;

          char buf[90];
          const size_t nbytes = sprintf(buf, (
            "*3\r\n"
              "+Allocated: %d\r\n"
              "+Filled: %d\r\n"
              "+All (includes next count): %d\r\n"
          ), table->size.allocated, table->size.filled, table->size.all);

          _write(entry.client, buf, nbytes);
        } else {
          _write(entry.client, "-Invalid type for 'HLEN' command\r\n", 34);
        }
      } else WRITE_NULL_REPLY(entry.client);
    } else {
      _write(entry.client, "-Not allowed to use this command, need P_READ\r\n", 47);
    }
  }
}

const struct Command cmd_hlen = {
  .name = "HLEN",
  .summary = "Returns field count information of the hash table.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
