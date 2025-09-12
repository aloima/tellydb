#include <telly.h>

#include <stdlib.h>
#include <stdbool.h>

static string_t run(struct CommandEntry entry) {
  if (entry.data->arg_count != 2) {
    PASS_NO_CLIENT(entry.client);
    return WRONG_ARGUMENT_ERROR("DECRBY");
  }

  if (!is_integer(entry.data->args[1].value)) {
    PASS_NO_CLIENT(entry.client);
    return RESP_ERROR_MESSAGE("Second argument must be an integer");
  }

  const string_t key = entry.data->args[0];
  struct KVPair *result = get_data(entry.database, key);
  const int64_t value = strtoll(entry.data->args[1].value, NULL, 10);
  int64_t *number;

  if (!result) {
    number = malloc(sizeof(int64_t));
    *number = -value;

    const bool success = set_data(entry.database, NULL, key, number, TELLY_NUM);

    if (!success) {
      free(number);

      PASS_NO_CLIENT(entry.client);
      return RESP_ERROR();
    }
  } else if (result->type == TELLY_NUM) {
    number = result->value;
    *number -= value;
  } else {
    return INVALID_TYPE_ERROR("DECRBY");
  }

  PASS_NO_CLIENT(entry.client);
  const size_t nbytes = create_resp_integer(entry.buffer, *number);
  return CREATE_STRING(entry.buffer, nbytes);
}

const struct Command cmd_decrby = {
  .name = "DECRBY",
  .summary = "Decrements the number stored at key by value.",
  .since = "0.2.0",
  .complexity = "O(1)",
  .permissions = (P_READ | P_WRITE),
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
