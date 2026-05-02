#include <telly.h>

static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count == 0) return;

  for (uint32_t i = 0; i < entry->args->count; ++i) {
    (void) insert_into_vector(server->keyspace, &entry->args->data[i]);
  }
}



static string_t run(struct CommandEntry *entry) {
  if (entry->args->count == 0) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("INCR");
  }

  uint32_t at;

  if (entry->client) {
    switch (entry->client->protover) {
      case RESP2: {
        entry->client->write_buf[0] = '*';
        at = ltoa(entry->args->count * 2, entry->client->write_buf + 1) + 1;
        entry->client->write_buf[at++] = '\r';
        entry->client->write_buf[at++] = '\n';
        break;
      }

      case RESP3:
        entry->client->write_buf[0] = '%';
        at = ltoa(entry->args->count, entry->client->write_buf + 1) + 1;
        entry->client->write_buf[at++] = '\r';
        entry->client->write_buf[at++] = '\n';
        break;
    }

    for (uint32_t i = 0; i < entry->args->count; ++i) {
      const string_t key = entry->args->data[i];
      struct KVPair *result = get_data(entry->database, key);
      void *number;
      at += create_resp_string(entry->client->write_buf + at, key);

      if (!result) {
        number = malloc(sizeof(mpz_t));
        if (number == NULL) {
          at += create_resp_string(entry->client->write_buf + at, CREATE_STRING("error", 5));
          continue;
        }

        mpz_t *value = number;
        mpz_init_set_ui(*value, 0);

        const bool success = set_data(entry->database, NULL, key, value, TELLY_INT, NULL);

        if (!success) {
          at += create_resp_string(entry->client->write_buf + at, CREATE_STRING("error", 5));
          mpz_clear(*value);
          free(number);
          continue;
        }

        at += create_resp_integer_mpz(entry->client->protover, entry->client->write_buf + at, *value);
      } else {
        switch (result->type) {
          case TELLY_INT: {
            number = result->value;

            mpz_t *value = number;
            mpz_add_ui(*value, *value, 1);
            at += create_resp_integer_mpz(entry->client->protover, entry->client->write_buf + at, *value);
            break;
          }

          case TELLY_DOUBLE: {
            number = result->value;

            mpf_t *value = number;
            mpf_add_ui(*value, *value, 1);
            at += create_resp_integer_mpf(entry->client->protover, entry->client->write_buf + at, *value);
            break;
          }

          default:
            at += create_resp_string(entry->client->write_buf + at, CREATE_STRING("invalid type", 12));
            break;
        }
      }
    }
  } else {
    for (uint32_t i = 0; i < entry->args->count; ++i) {
      const string_t key = entry->args->data[i];
      struct KVPair *result = get_data(entry->database, key);
      void *number;
      at += create_resp_string(entry->client->write_buf + at, key);

      if (!result) {
        number = malloc(sizeof(mpz_t));
        if (number == NULL) continue;

        mpz_init_set_ui(*((mpz_t *) number), 0);
        const bool success = set_data(entry->database, NULL, key, number, TELLY_INT, NULL);

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
            mpz_add_ui(*value, *value, 1);
            break;
          }

          case TELLY_DOUBLE: {
            number = result->value;

            mpf_t *value = number;
            mpf_add_ui(*value, *value, 1);
            break;
          }

          default:
            break;
        }
      }
    }
  }

  PASS_NO_CLIENT(entry->client);
  return CREATE_STRING(entry->client->write_buf, at);
}

const struct Command cmd_incr = {
  .name = "INCR",
  .summary = "Increments the number stored at each key.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .permissions = (P_READ | P_WRITE),
  .flags.value = (CMD_FLAG_ACCESS_DATABASE | CMD_FLAG_AFFECT_DATABASE),
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
