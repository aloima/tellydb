#include <telly.h>

#include <stdlib.h>
#include <stdbool.h>

#include <gmp.h>

static string_t run(struct CommandEntry entry) {
  if (entry.data->arg_count != 2) {
    PASS_NO_CLIENT(entry.client);
    return WRONG_ARGUMENT_ERROR("DECRBY");
  }

  const char *input = entry.data->args[1].value;

  if (!try_parse_integer(input) && !try_parse_double(input)) {
    PASS_NO_CLIENT(entry.client);
    return RESP_ERROR_MESSAGE("Second argument must be an integer or a double");
  }

  const string_t key = entry.data->args[0];
  struct KVPair *result = get_data(entry.database, key);

  mpf_t *number, value;

  if (!result) {
    number = malloc(sizeof(mpf_t));
    mpf_init2(*number, FLOAT_PRECISION);
    mpf_set_str(*number, input, 10);

    const bool success = set_data(entry.database, NULL, key, number, TELLY_NUM);

    if (!success) {
      mpf_clear(*number);
      free(number);

      PASS_NO_CLIENT(entry.client);
      return RESP_ERROR();
    }
  } else if (result->type == TELLY_NUM) {
    mpf_init2(value, FLOAT_PRECISION);
    mpf_set_str(value, input, 10);

    number = result->value;
    mpf_sub(*number, *number, value);
    mpf_clear(value);
  } else {
    return INVALID_TYPE_ERROR("DECRBY");
  }

  PASS_NO_CLIENT(entry.client);
  const size_t nbytes = create_resp_integer_mpf(entry.client->protover, entry.buffer, *number);
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
