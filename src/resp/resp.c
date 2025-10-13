#include <telly.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static inline bool check_crlf(struct Client *client, char *buf, int32_t *at, int32_t *size) {
  char crlf[2];
  TAKE_BYTES(crlf, 2, false);

  return !VERY_UNLIKELY(crlf[0] != '\r' || crlf[1] != '\n');
}

static inline bool get_resp_command_name(struct Client *client, commandname_t *name, char *buf, int32_t *at, int32_t *size) {
  char c;
  TAKE_BYTES(&c, 1, false);

  if (VERY_UNLIKELY(c != RDT_BSTRING)) {
    throw_resp_error(client->id);
    return false;
  }

  TAKE_BYTES(&c, 1, false);

  if (VERY_UNLIKELY(!('0' <= c && c <= '9'))) {
    throw_resp_error(client->id);
    return false;
  }

  do {
    name->len = (name->len * 10) + (c - '0');
    TAKE_BYTES(&c, 1, false);
  } while ('0' <= c && c <= '9');

  if (VERY_UNLIKELY(c != '\r')) {
    throw_resp_error(client->id);
    return false;
  }

  TAKE_BYTES(&c, 1, false);

  if (VERY_UNLIKELY(c != '\n')) {
    throw_resp_error(client->id);
    return false;
  }

  if (name->len > COMMAND_NAME_MAX_LENGTH) {
    TAKE_BYTES(name->value, COMMAND_NAME_MAX_LENGTH - 1, false);
    name->value[name->len] = '\0';

    TAKE_BYTES(NULL, name->len - (COMMAND_NAME_MAX_LENGTH - 1), false);
  } else {
    TAKE_BYTES(name->value, name->len, false);
    name->value[name->len] = '\0';
  }

  if (!check_crlf(client, buf, at, size)) {
    throw_resp_error(client->id);
    return false;
  }

  return true;
}

static inline bool get_resp_command_argument(struct Client *client, string_t *argument, char *buf, int32_t *at, int32_t *size) {
  char c;
  TAKE_BYTES(&c, 1, false);

  if (VERY_UNLIKELY(c != RDT_BSTRING)) {
    throw_resp_error(client->id);
    return false;
  }

  TAKE_BYTES(&c, 1, false);

  if (VERY_UNLIKELY(!('0' <= c && c <= '9'))) {
    throw_resp_error(client->id);
    return false;
  }

  do {
    argument->len = (argument->len * 10) + (c - '0');
    TAKE_BYTES(&c, 1, false);
  } while ('0' <= c && c <= '9');

  if (VERY_UNLIKELY(c != '\r')) {
    throw_resp_error(client->id);
    return false;
  }

  TAKE_BYTES(&c, 1, false);

  // better without VERY_UNLIKELY(), why??
  if (c != '\n') {
    throw_resp_error(client->id);
    return false;
  }

  if (VERY_UNLIKELY((argument->value = malloc(argument->len + 1)) == NULL)) {
    throw_resp_error(client->id);
    return false;
  }

  TAKE_BYTES(argument->value, argument->len, false);
  argument->value[argument->len] = '\0';

  if (!check_crlf(client, buf, at, size)) {
    throw_resp_error(client->id);
    return false;
  }

  return true;
}

bool parse_resp_command(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command) {
  command->arg_count = 0;
  command->args = NULL;

  char c;
  TAKE_BYTES(&c, 1, false);

  if (VERY_UNLIKELY(!('0' <= c && c <= '9'))) {
    throw_resp_error(client->id);
    return false;
  }

  do {
    command->arg_count = (command->arg_count * 10) + (c - '0');
    TAKE_BYTES(&c, 1, false);
  } while ('0' <= c && c <= '9');

  if (VERY_UNLIKELY(c != '\r')) {
    throw_resp_error(client->id);
    return false;
  }

  TAKE_BYTES(&c, 1, false);

  if (VERY_UNLIKELY(c != '\n')) {
    throw_resp_error(client->id);
    return false;
  }

  if (command->arg_count == 0) {
    write_log(LOG_ERR, "Received data from Client #%u is empty RESP data, so it cannot be created as a command.", client->id);
    return false;
  }

  command->name.len = 0;

  if (!get_resp_command_name(client, &command->name, buf, at, size)) {
    return false;
  }

  command->arg_count -= 1;

  if (command->arg_count != 0) {
    command->args = malloc(command->arg_count * sizeof(string_t));

    for (uint32_t i = 0; i < command->arg_count; ++i) {
      command->args[i].len = 0;
      command->args[i].value = NULL;

      if (VERY_UNLIKELY(!get_resp_command_argument(client, &command->args[i], buf, at, size))) {
        for (uint32_t j = 0; j < i; ++j) {
          free(command->args[j].value);
        }

        if (command->args[i].value) {
          free(command->args[i].value);
        }

        free(command->args);
        return false;
      }
    }
  }

  return true;
}
