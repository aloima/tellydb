#include <telly.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static string_t run(struct CommandEntry entry) {
  if (entry.data->arg_count != 1) {
    PASS_NO_CLIENT(entry.client);
    return WRONG_ARGUMENT_ERROR("DECR");
  }

  const string_t key = entry.data->args[0];
  struct KVPair *result = get_data(entry.database, key);

  if (!result) {
    long *number = calloc(1, sizeof(long));
    const bool success = set_data(entry.database, NULL, key, number, TELLY_NUM);

    if (entry.client) {
      if (success) {
        return CREATE_STRING(":0\r\n", 4);
      } else {
        return RESP_ERROR();
      }
    }
  } else if (result->type == TELLY_NUM) {
    long *number = result->value;
    *number -= 1;

    PASS_NO_CLIENT(entry.client);
    const size_t nbytes = create_resp_integer(entry.buffer, *number);
    return CREATE_STRING(entry.buffer, nbytes);
  }

  PASS_NO_CLIENT(entry.client);
  return INVALID_TYPE_ERROR("DECR");
}

const struct Command cmd_decr = {
  .name = "DECR",
  .summary = "Decrements value.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .permissions = (P_READ | P_WRITE),
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
