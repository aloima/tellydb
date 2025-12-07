#include <telly.h>

#include <stdlib.h>
#include <stdbool.h>

#include <gmp.h>

static string_t run(struct CommandEntry *entry) {
  if (entry->data->arg_count != 2) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("DECRBY");
  }

  const char *input = entry->data->args[1].value;
  const bool is_integer = try_parse_integer(input);
  const bool is_double = try_parse_double(input);

  if (!is_integer && !is_double) {
    PASS_NO_CLIENT(entry->client);
    return RESP_ERROR_MESSAGE("Second argument must be an integer or double");
  }

  const string_t key = entry->data->args[0];
  struct KVPair *result = get_data(entry->database, key);

  if (!result) {
    void *number;
    enum TellyTypes type;

    if (is_integer) {
      type = TELLY_INT;
      mpz_t *raw = (number = malloc(sizeof(mpz_t)));
      mpz_init_set_str(*raw, input, 10);
      mpz_neg(*raw, *raw);
    } else if (is_double) {
      type = TELLY_DOUBLE;
      mpf_t *raw = (number = malloc(sizeof(mpf_t)));
      mpf_init2(*raw, FLOAT_PRECISION);
      mpf_set_str(*raw, input, 10);
      mpf_neg(*raw, *raw);
    }

    result = set_data(entry->database, NULL, key, number, type);

    if (!result) {
      free_value(type, number);
      PASS_NO_CLIENT(entry->client);
      return RESP_ERROR();
    }
  } else {
    switch (result->type) {
      case TELLY_INT:
        if (is_integer) {
          mpz_t value;
          mpz_init_set_str(value, input, 10);

          result->type = TELLY_INT;
          mpz_t *raw = result->value;
          mpz_sub(*raw, *raw, value);
          mpz_clear(value);
        } else {
          mpf_t original;
          mpf_init2(original, FLOAT_PRECISION);
          mpf_set_z(original, result->value);

          mpz_clear(result->value);
          free(result->value);

          result->type = TELLY_DOUBLE;
          mpf_t *raw = (result->value = malloc(sizeof(mpf_t)));
          mpf_init2(*raw, FLOAT_PRECISION);
          mpf_set_str(*raw, input, 10);
          mpf_sub(*raw, *raw, original);
          mpf_neg(*raw, *raw); 
          mpf_clear(original);
        }

        break;

      case TELLY_DOUBLE: {
        mpf_t value;
        mpf_init2(value, FLOAT_PRECISION);
        mpf_set_str(value, input, 10);

        result->type = TELLY_DOUBLE;
        mpf_t *raw = result->value;
        mpf_sub(*raw, *raw, value);
        mpf_clear(value);
        break;
      }

      default:
        return INVALID_TYPE_ERROR("DECRBY");
    }
  }

  PASS_NO_CLIENT(entry->client);
  return write_value(result->value, result->type, entry->client->protover, entry->client->write_buf);
}

const struct Command cmd_decrby = {
  .name = "DECRBY",
  .summary = "Decrements the number stored at key by value.",
  .since = "0.2.0",
  .complexity = "O(1)",
  .permissions = (P_READ | P_WRITE),
  .flags = CMD_FLAG_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
