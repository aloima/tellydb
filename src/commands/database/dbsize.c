#include <telly.h>

#include <stdio.h>
#include <stdint.h>

static string_t run(struct CommandEntry entry) {
  PASS_NO_CLIENT(entry.client);

  struct BTree *cache;

  if (entry.data->arg_count != 1) {
    cache = entry.database->cache;
  } else {
    struct BTree *found = get_cache_of_database(entry.data->args[0]);

    if (found) {
      cache = found;
    } else {
      return RESP_ERROR_MESSAGE("Database cannot be found");
    }
  }

  const size_t nbytes = create_resp_integer(entry.buffer, cache->size);
  return CREATE_STRING(entry.buffer, nbytes);
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
