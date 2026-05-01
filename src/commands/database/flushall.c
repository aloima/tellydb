#include <telly.h>

static string_t run(struct CommandEntry *entry) {
  LinkedListNode *node = get_databases()->begin;

  while (node) {
    Database *database = (Database *) node->data;
    clear_database(database);

    node = node->next;
  }

  PASS_NO_CLIENT(entry->client);
  return RESP_OK();
}

const struct Command cmd_flushall = {
  .name = "FLUSHALL",
  .summary = "Deletes all the keys of all databases.",
  .since = "1.0.0",
  .complexity = "O(N) where N is total number of the keys in all databases",
  .permissions = P_WRITE,
  .flags.value = CMD_FLAG_AFFECT_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = NULL
};
