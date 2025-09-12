#include <telly.h>

#include <stdlib.h>
#include <stdbool.h>

static string_t run(struct CommandEntry entry) {
  if (entry.data->arg_count == 0) {
    PASS_NO_CLIENT(entry.client);
    return WRONG_ARGUMENT_ERROR("INCR");
  }

  uint32_t at;

  if (entry.client) {
    switch (entry.client->protover) {
      case RESP2: {
        entry.buffer[0] = '*';
        at = ltoa(entry.data->arg_count * 2, entry.buffer + 1) + 1;
        entry.buffer[at++] = '\r';
        entry.buffer[at++] = '\n';
        break;
      }

      case RESP3:
        entry.buffer[0] = '%';
        at = ltoa(entry.data->arg_count, entry.buffer + 1) + 1;
        entry.buffer[at++] = '\r';
        entry.buffer[at++] = '\n';
        break;
    }

    for (uint32_t i = 0; i < entry.data->arg_count; ++i) {
      const string_t key = entry.data->args[i];
      struct KVPair *result = get_data(entry.database, key);
      long *number;
      at += create_resp_string(entry.buffer + at, key);

      if (!result) {
        number = calloc(1, sizeof(long));
        const bool success = set_data(entry.database, NULL, key, number, TELLY_NUM);

        if (!success) {
          at += create_resp_string(entry.buffer + at, CREATE_STRING("error", 5));
          free(number);
          continue;
        }
      } else if (result->type == TELLY_NUM) {
        number = result->value;
        *number += 1;
      } else {
        at += create_resp_string(entry.buffer + at, CREATE_STRING("invalid type", 12));
        continue;
      }

      at += create_resp_integer(entry.buffer + at, *number);
    }
  } else {
    for (uint32_t i = 0; i < entry.data->arg_count; ++i) {
      const string_t key = entry.data->args[i];
      struct KVPair *result = get_data(entry.database, key);
      long *number;
      at += create_resp_string(entry.buffer + at, key);

      if (!result) {
        number = calloc(1, sizeof(long));
        const bool success = set_data(entry.database, NULL, key, number, TELLY_NUM);

        if (!success) {
          free(number);
          continue;
        }
      } else if (result->type == TELLY_NUM) {
        number = result->value;
        *number += 1;
      } else {
        continue;
      }

      at += create_resp_integer(entry.buffer + at, *number);
    }
  }

  PASS_NO_CLIENT(entry.client);
  return CREATE_STRING(entry.buffer, at);
}

const struct Command cmd_incr = {
  .name = "INCR",
  .summary = "Increments value(s).",
  .since = "0.1.0",
  .complexity = "O(1)",
  .permissions = (P_READ | P_WRITE),
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
