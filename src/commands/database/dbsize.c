#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>

static void run(struct CommandEntry entry) {
  if (!entry.client) return;
  struct BTree *cache = entry.database->cache;

  if (entry.data->arg_count == 1) {
    struct BTree *found = get_cache_of_database(entry.data->args[0]);

    if (found) cache = found;
    else {
      _write(entry.client, "-Database cannot be found\r\n", 27);
      return;
    }
  }

  char buf[14];
  const size_t nbytes = sprintf(buf, ":%d\r\n", cache->size);

  _write(entry.client, buf, nbytes);
}

const struct Command cmd_dbsize = {
  .name = "DBSIZE",
  .summary = "Returns key count in the database.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .permissions = P_READ,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
