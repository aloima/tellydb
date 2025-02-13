#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (command->arg_count == 0) {
    if (client) WRONG_ARGUMENT_ERROR(client, "DEL", 3);
    return;
  }

  if (password->permissions & P_WRITE) {
    uint32_t deleted = 0;

    for (uint32_t i = 0; i < command->arg_count; ++i) {
      deleted += delete_data(command->args[i]);
    }

    if (client) {
      char res[13];
      const size_t res_len = sprintf(res, ":%d\r\n", deleted);

      _write(client, res, res_len);
    }
  } else if (client) {
    _write(client, "-Not allowed to use this command, need P_WRITE\r\n", 48);
  }
}

const struct Command cmd_del = {
  .name = "DEL",
  .summary = "Deletes the specified keys.",
  .since = "0.1.7",
  .complexity = "O(N) where N is key count",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
