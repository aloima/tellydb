#include <telly.h>

#include <stddef.h>
#include <stdbool.h>

static inline bool parse_name(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command, char *c) {
  uint8_t idx = 0;

  if (*c == ' ') {
    return false;
  }

  do {
    command->name.value[idx] = *c;

    if (take_n_bytes_from_socket(client, buf, at, c, 1, size) != 1) {
      idx += 1;
      command->name.value[idx] = '\0';
      command->name.len = idx;
      return true;
    }

    idx += 1;
  } while (*c != ' ' && *c != '\r' && *c != '\n');

  command->name.value[idx] = '\0';
  command->name.len = idx;
  return true;
}

static inline bool parse_arguments(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command, char *c) {
  bool retrieving = true;

  while (retrieving) {
    uint8_t idx = 0;
    command->arg_count += 1;

    if (!command->args) {
      command->args = calloc(1, sizeof(string_t));
    } else {
      command->args = realloc(command->args, sizeof(string_t) * command->arg_count);
    }

    string_t *arg = &command->args[command->arg_count - 1];
    arg->len = 0;
    arg->value = NULL;

    char value[128];
    TAKE_BYTES(c, 1, false);

    while (*c != ' ') {
      value[idx] = *c;
      idx += 1;

      if (idx == 128) {
        idx = 0;

        if (arg->value == NULL) {
          arg->value = malloc(128 * sizeof(char));
        } else {
          arg->value = realloc(arg->value, (arg->len + 128) * sizeof(char));
        }

        arg->len += 128;
        memcpy(arg->value, value, 128);
      }

      TAKE_BYTES(c, 1, false);

      if (*c == '\r') {
        TAKE_BYTES(NULL, 1, false);
        retrieving = false;
        break;
      }

      if (*c == '\n') {
        retrieving = false;
        break;
      }
    }

    if (idx != 0) {
      if (arg->value == NULL) {
        arg->value = malloc((idx + 1) * sizeof(char));
        memcpy(arg->value, value, idx);
      } else {
        arg->value = realloc(arg->value, (arg->len + idx + 1) * sizeof(char));
        memcpy(arg->value + arg->len, value, idx);
      }

      arg->value[idx] = '\0';
      arg->len += idx;
    }
  }

  return true;
}

bool parse_inline_command(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command, char c) {
  command->args = NULL;
  command->arg_count = 0;

  if (!parse_name(client, buf, at, size, command, &c)) {
    throw_resp_error(client->id);
    return false;
  }

  if (c == '\n') {
    return true;
  }

  if (c == '\r') {
    TAKE_BYTES(NULL, 1, false);
    return true;
  }

  if (!parse_arguments(client, buf, at, size, command, &c)) {
    throw_resp_error(client->id);
    return false;
  }

  return true;
}
