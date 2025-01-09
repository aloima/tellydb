#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/utils.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (command->arg_count != 1) {
    if (client) WRONG_ARGUMENT_ERROR(client, "DECR", 4);
    return;
  }

  if (password->permissions & (P_READ | P_WRITE)) {
    string_t key = command->args[0];
    struct KVPair *result = get_data(key);

    if (!result) {
      long *number = calloc(1, sizeof(long));
      const bool success = set_data(NULL, key, number, TELLY_NUM);

      if (client) {
        if (success) _write(client, ":0\r\n", 4);
        else WRITE_ERROR(client);
      }
    } else if (result->type == TELLY_NUM) {
      long *number = result->value;
      *number -= 1;

      if (client) {
        char buf[24];
        const size_t nbytes = sprintf(buf, ":%ld\r\n", *number);
        _write(client, buf, nbytes);
      }
    } else if (client) {
      _write(client, "-Invalid type for 'DECR' command\r\n", 34);
    }
  } else {
    _write(client, "-Not allowed to use this command, need P_READ and P_WRITE\r\n", 59);
  }
}

const struct Command cmd_decr = {
  .name = "DECR",
  .summary = "Decrements value.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
