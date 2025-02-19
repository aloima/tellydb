#include "../../../headers/telly.h"

#include <stdint.h>
#include <stddef.h>

static void run(struct CommandEntry entry) {
  if (entry.client) {
    if (entry.data->arg_count == 1) {
      struct LinkedListNode *node = get_database_node();
      const uint64_t target = hash(entry.data->args[0].value, entry.data->args[0].len);

      while (node) {
        struct Database *database = node->data;

        if (database->id == target) {
          entry.client->database = database;
          WRITE_OK(entry.client);
          return;
        }

        node = node->next;
      }

      _write(entry.client, "-This database cannot be found\r\n", 32);
    } else {
      _write(entry.client, "-Invalid command usage\r\n", 24);
    }
  }
}

const struct Command cmd_select = {
  .name = "SELECT",
  .summary = "Selects database which will be used by client.",
  .since = "0.1.9",
  .complexity = "O(N) where N is total database count",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
