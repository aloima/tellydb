#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

static void run(struct Client *client, commanddata_t *command, __attribute__((unused)) struct Password *password) {
  if (client) {
    if (command->arg_count == 0) {
      WRONG_ARGUMENT_ERROR(client, "EXISTS", 6);
      return;
    }

    uint32_t existed = 0, not_existed = 0;

    char buf[8192];
    buf[0] = '\0';

    for (uint32_t i = 0; i < command->arg_count; ++i) {
      const char *key = command->args[i].value;

      if (get_data(key)) {
        existed += 1;
        strcat(buf, "+exists\r\n");
      } else {
        not_existed += 1;
        strcat(buf, "+not exist\r\n");
      }
    }

    char res[85 + (existed * 9) + (not_existed * 12)];
    const size_t nbytes = sprintf(res, (
      "*%d\r\n"
        "+existed key count is %d\r\n"
        "+not existed key count is %d\r\n"
        "%s"
    ), command->arg_count + 2, existed, not_existed, buf);

    _write(client, res, nbytes);
  }
}

struct Command cmd_exists = {
  .name = "EXISTS",
  .summary = "Checks if specified keys exist or not.",
  .since = "0.1.4",
  .complexity = "O(N) where N is key count",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
