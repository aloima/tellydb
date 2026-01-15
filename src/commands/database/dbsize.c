#include <telly.h>

#include <stddef.h>
#include <stdint.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  struct Database *database;

  if (entry->args->count != 1) {
    database = entry->database;
  } else {
    database = get_database(entry->args->data[0]);

    if (!database) {
      return RESP_ERROR_MESSAGE("Database cannot be found");
    }
  }

  const size_t nbytes = create_resp_integer(entry->client->write_buf, database->size.stored);
  return CREATE_STRING(entry->client->write_buf, nbytes);
}

const struct Command cmd_dbsize = {
  .name = "DBSIZE",
  .summary = "Returns key count in the database.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .permissions = P_READ,
  .flags.value = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
