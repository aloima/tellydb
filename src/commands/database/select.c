#include <telly.h>

#include <stdint.h>
#include <stddef.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 1) {
    return WRONG_ARGUMENT_ERROR("SELECT");
  }

  struct LinkedListNode *node = get_database_node();
  const uint64_t target = hash(entry->args->data[0].value, entry->args->data[0].len);

  while (node) {
    struct Database *database = node->data;

    if (database->id == target) {
      entry->client->database = database;
      return RESP_OK();
    }

    node = node->next;
  }

  struct Database *database = create_database(entry->args->data[0], DATABASE_INITIAL_SIZE);
  if (database == NULL) return RESP_ERROR_MESSAGE("Memory error, cannot create new database");

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
