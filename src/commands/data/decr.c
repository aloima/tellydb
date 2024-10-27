#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/utils.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

static void run(struct Client *client, commanddata_t *command) {
  if (command->arg_count != 1) {
    if (client) WRONG_ARGUMENT_ERROR(client, "DECR", 4);
    return;
  }

  string_t key = command->args[0];
  struct KVPair *result = get_data(key.value);

  if (!result) {
    long *number = calloc(1, sizeof(long));
    set_data(NULL, key, number, TELLY_NUM);

    if (client) _write(client, ":0\r\n", 4);
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
}

struct Command cmd_decr = {
  .name = "DECR",
  .summary = "Decrements value from specified key.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
