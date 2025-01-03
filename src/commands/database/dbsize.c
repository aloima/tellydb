#include "../../../headers/server.h"
#include "../../../headers/btree.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stdio.h>
#include <stdint.h>

static void run(struct Client *client, __attribute__((unused)) commanddata_t *command, struct Password *password) {
  if (client) {
    if (password->permissions & P_READ) {
      struct BTree *cache = get_cache();

      char buf[14];
      const size_t nbytes = sprintf(buf, ":%d\r\n", cache->size);

      _write(client, buf, nbytes);
    } else {
      _write(client, "-Not allowed to use this command, need P_READ\r\n", 47);
    }
  }
}

const struct Command cmd_dbsize = {
  .name = "DBSIZE",
  .summary = "Returns key count in the database.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
