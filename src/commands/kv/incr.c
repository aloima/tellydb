#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

static void run(struct CommandEntry entry) {
  if (entry.data->arg_count != 1) {
    if (entry.client) WRONG_ARGUMENT_ERROR(entry.client, "INCR", 4);
    return;
  }

  if (entry.password->permissions & (P_READ | P_WRITE)) {
    const string_t key = entry.data->args[0];
    struct KVPair *result = get_data(entry.database, key);

    if (!result) {
      long *number = calloc(1, sizeof(long));
      const bool success = set_data(entry.database, NULL, key, number, TELLY_NUM);

      if (entry.client) {
        if (success) _write(entry.client, ":0\r\n", 4);
        else WRITE_ERROR(entry.client);
      }
    } else if (result->type == TELLY_NUM) {
      long *number = result->value;
      *number += 1;

      if (entry.client) {
        char buf[24];
        const size_t nbytes = sprintf(buf, ":%ld\r\n", *number);
        _write(entry.client, buf, nbytes);
      }
    } else if (entry.client) {
      _write(entry.client, "-Invalid type for 'INCR' command\r\n", 34);
    }
  } else if (entry.client) {
    _write(entry.client, "-Not allowed to use this command, need P_READ and P_WRITE\r\n", 59);
  }
}

const struct Command cmd_incr = {
  .name = "INCR",
  .summary = "Increments value.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
