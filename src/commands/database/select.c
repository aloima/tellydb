#include <telly.h>

#include <stdint.h>
#include <stddef.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 1) {
    return WRONG_ARGUMENT_ERROR("SELECT");
  }

  const string_t name = entry->args->data[0];

  Database *database = get_database(name);

  if (database == NULL) {
    database = create_database(name, DATABASE_INITIAL_SIZE);
    if (database == NULL) return RESP_ERROR_MESSAGE("Memory error, cannot create new database");
  }

  entry->client->database = database;
  return RESP_OK();
}

const struct Command cmd_select = {
  .name = "SELECT",
  .summary = "Selects database which will be used by client.",
  .since = "0.1.9",
  .complexity = "O(N) where N is total database count",
  .permissions = P_NONE,
  .flags.value = CMD_FLAG_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
