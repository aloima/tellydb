#include "../../../headers/server.h"
#include "../../../headers/commands.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (client) {
    if (command->arg_count != 1 && command->arg_count != 2) {
      WRONG_ARGUMENT_ERROR(client, "AUTH", 4);
      return;
    }

    const char *value = command->args[0].value;
    const int32_t at = where_password(value);

    if (at == -1) {
      _write(client, "-This password does not exist\r\n", 31);
    } else {
      if (password) {
        if (command->arg_count != 2) {
          _write(client, "+A password already in use for your client. If you sure to change, use command with ok argument\r\n", 97);
          return;
        } else {
          const char *ok = command->args[1].value;

          if (!streq(ok, "ok")) {
            _write(client, "+A password already in use for your client. If you sure to change, use command with ok argument\r\n", 97);
            return;
          }
        }
      }

      struct Password **passwords = get_passwords();
      client->password = passwords[at];

      WRITE_OK(client);
    }
  }
}

struct Command cmd_auth = {
  .name = "AUTH",
  .summary = "Allows to authorize client via passwords.",
  .since = "0.1.7",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
