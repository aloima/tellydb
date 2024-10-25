#include "../../../headers/server.h"
#include "../../../headers/btree.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stdio.h>
#include <stdint.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count != 1) {
      WRONG_ARGUMENT_ERROR(client, "DBSIZE", 6);
      return;
    }

    struct BTree *cache = get_cache();

    char buf[14];
    const size_t nbytes = sprintf(buf, ":%d\r\n", cache->size);

    _write(client, buf, nbytes);
  }
}

struct Command cmd_dbsize = {
  .name = "DBSIZE",
  .summary = "Returns key count in the database.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
