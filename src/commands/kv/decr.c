#include <telly.h>

#include <stdlib.h>
#include <stdbool.h>

#include <gmp.h>

static string_t run(struct CommandEntry *entry) {
  if (entry->data->arg_count == 0) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("INCR");
  }

  uint32_t at;

  if (entry->client) {
    switch (entry->client->protover) {
      case RESP2: {
        entry->buffer[0] = '*';
        at = ltoa(entry->data->arg_count * 2, entry->buffer + 1) + 1;
        entry->buffer[at++] = '\r';
        entry->buffer[at++] = '\n';
        break;
      }

      case RESP3:
        entry->buffer[0] = '%';
        at = ltoa(entry->data->arg_count, entry->buffer + 1) + 1;
        entry->buffer[at++] = '\r';
        entry->buffer[at++] = '\n';
        break;
    }

    for (uint32_t i = 0; i < entry->data->arg_count; ++i) {
      const string_t key = entry->data->args[i];
      struct KVPair *result = get_data(entry->database, key);
      void *number;
      at += create_resp_string(entry->buffer + at, key);

      if (!result) {
        number = malloc(sizeof(mpz_t));

        mpz_t *value = number;
        mpz_init_set_ui(*value, 0);

        const bool success = set_data(entry->database, NULL, key, value, TELLY_INT);

        if (!success) {
          at += create_resp_string(entry->buffer + at, CREATE_STRING("error", 5));
          mpz_clear(*value);
          free(number);
          continue;
        }

        at += create_resp_integer_mpz(entry->client->protover, entry->buffer + at, *value);
      } else {
        switch (result->type) {
          case TELLY_INT: {
            number = result->value;

            mpz_t *value = number;
            mpz_sub_ui(*value, *value, 1);
            at += create_resp_integer_mpz(entry->client->protover, entry->buffer + at, *value);
            break;
          }

          case TELLY_DOUBLE: {
            number = result->value;

            mpf_t *value = number;
            mpf_sub_ui(*value, *value, 1);
            at += create_resp_integer_mpf(entry->client->protover, entry->buffer + at, *value);
            break;
          }

          default:
            at += create_resp_string(entry->buffer + at, CREATE_STRING("invalid type", 12));
            break;
        }
      }
    }
  } else {
    for (uint32_t i = 0; i < entry->data->arg_count; ++i) {
      const string_t key = entry->data->args[i];
      struct KVPair *result = get_data(entry->database, key);
      void *number;
      at += create_resp_string(entry->buffer + at, key);

      if (!result) {
        number = malloc(sizeof(mpz_t));
        mpz_init_set_ui(*((mpz_t *) number), 0);

        const bool success = set_data(entry->database, NULL, key, number, TELLY_INT);

        if (!success) {
          mpz_clear(*((mpz_t *) number));
          free(number);
          continue;
        }
      } else {
        switch (result->type) {
          case TELLY_INT: {
            number = result->value;

            mpz_t *value = number;
            mpz_sub_ui(*value, *value, 1);
            break;
          }

          case TELLY_DOUBLE: {
            number = result->value;

            mpf_t *value = number;
            mpf_sub_ui(*value, *value, 1);
            break;
          }

          default:
            break;
        }
      }
    }
  }

  PASS_NO_CLIENT(entry->client);
  return CREATE_STRING(entry->buffer, at);
}

const struct Command cmd_decr = {
  .name = "DECR",
  .summary = "Decrements the number stored at each key.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .permissions = (P_READ | P_WRITE),
  .flags = CMD_FLAG_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
