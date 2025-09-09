#include <telly.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#define DATA_ERR(client) \
  write_log(LOG_ERR, "Received data from Client #%u cannot be validated as a RESP data, so it cannot be created as a command.", (client)->id);

#define TAKE_BYTES_RETURN_VALUE(value, n, return_value) \
  if (take_n_bytes(client, buf, at, value, n, size) != n) { \
    DATA_ERR(client); \
    return return_value; \
  }

#define TAKE_BYTES(value, n) \
  if (take_n_bytes(client, buf, at, value, n, size) != n) { \
    DATA_ERR(client); \
    return; \
  }

static inline int take_n_bytes(struct Client *client, char *buf, int32_t *at, void *data, const uint32_t n, int32_t *size) {
  const uint32_t remaining = (RESP_BUF_SIZE - *at);

  if (VERY_LIKELY(*at < remaining)) {
    if (data != NULL) {
      memcpy_aligned(data, buf + *at, n);
    }

    *at += n;
    return n;
  }

  if (data != NULL) {
    memcpy_aligned(data, buf + *at, remaining);

    if (_read(client, data + remaining, n - remaining) <= 0) {
      return remaining;
    }
  } else {
    char dummy[128];
    uint32_t length = n - remaining;
    uint32_t quotient = length / 128;

    for (uint32_t i = 0; i < quotient; ++i) {
      _read(client, dummy, 128);
    }

    _read(client, dummy, length - (128 * quotient));
  }

  *at = 0;
  *size = 0;
  return n;
}

static inline void get_resp_command_name(struct Client *client, commandname_t *name, char *buf, int32_t *at, int32_t *size) {
  char c;
  TAKE_BYTES(&c, 1);

  if (VERY_UNLIKELY(c != RDT_BSTRING)) {
    DATA_ERR(client);
    return;
  }

  while (true) {
    TAKE_BYTES(&c, 1);

    if (isdigit(c)) {
      name->len = (name->len * 10) + (c - 48);
    } else if (c == '\r') {
      TAKE_BYTES(&c, 1);

      if (c != '\n') {
        DATA_ERR(client);
        return;
      }

      if (name->len > COMMAND_NAME_MAX_LENGTH) {
        TAKE_BYTES(name->value, COMMAND_NAME_MAX_LENGTH - 1);
        name->value[name->len] = '\0';

        TAKE_BYTES(NULL, name->len - (COMMAND_NAME_MAX_LENGTH - 1));
      } else {
        TAKE_BYTES(name->value, name->len);
        name->value[name->len] = '\0';
      }

      char crlf[2];
      TAKE_BYTES(crlf, 2);

      if (VERY_UNLIKELY(crlf[0] != '\r' || crlf[1] != '\n')) {
        DATA_ERR(client);
        return;
      }

      return;
    } else {
      DATA_ERR(client);
      return;
    }
  }
}

static inline void get_resp_command_argument(struct Client *client, string_t *argument, char *buf, int32_t *at, int32_t *size) {
  char c;
  TAKE_BYTES(&c, 1);

  if (VERY_UNLIKELY(c != RDT_BSTRING)) {
    DATA_ERR(client);
    return;
  }

  while (true) {
    TAKE_BYTES(&c, 1);

    if (isdigit(c)) {
      argument->len = (argument->len * 10) + (c - 48);
    } else if (c == '\r') {
      TAKE_BYTES(&c, 1);

      if (c != '\n') {
        DATA_ERR(client);
        return;
      }

      argument->value = malloc(argument->len + 1);
      TAKE_BYTES(argument->value, argument->len);
      argument->value[argument->len] = '\0';

      char crlf[2];
      TAKE_BYTES(crlf, 2);

      if (VERY_UNLIKELY(crlf[0] != '\r' || crlf[1] != '\n')) {
        DATA_ERR(client);
        free(argument->value);
        return;
      }

      return;
    } else {
      DATA_ERR(client);
      return;
    }
  }
}

static bool parse_resp_command(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command) {
  command->arg_count = 0;
  command->args = NULL;

  while (true) {
    char c;
    TAKE_BYTES_RETURN_VALUE(&c, 1, false);

    if (isdigit(c)) {
      command->arg_count = (command->arg_count * 10) + (c - 48);
    } else if (c == '\r') {
      TAKE_BYTES_RETURN_VALUE(&c, 1, false);

      if (c != '\n') {
        DATA_ERR(client);
        return false;
      }

      if (command->arg_count == 0) {
        write_log(LOG_ERR, "Received data from Client #%u is empty RESP data, so it cannot be created as a command.", client->id);
        return false;
      }

      command->name.len = 0;
      get_resp_command_name(client, &command->name, buf, at, size);
      command->arg_count -= 1;

      if (command->arg_count != 0) {
        command->args = malloc(command->arg_count * sizeof(string_t));

        for (uint32_t i = 0; i < command->arg_count; ++i) {
          command->args[i].len = 0;
          get_resp_command_argument(client, &command->args[i], buf, at, size);
        }
      }

      return true;
    } else {
      DATA_ERR(client);
      return false;
    }
  }
}

bool get_command_data(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command) {
  uint8_t type;
  TAKE_BYTES_RETURN_VALUE(&type, 1, false);

  if (VERY_LIKELY(type == RDT_ARRAY)) {
    if (parse_resp_command(client, buf, at, size, command)) {
      return true;
    }

    return false;
  } else {
    write_log(LOG_ERR, "Received data from Client #%u is not RESP array, so it cannot be read as a command.", client->id);
    return false;
  }
}

void free_command_data(commanddata_t command) {
  if (command.arg_count != 0) {
    for (uint32_t i = 0; i < command.arg_count; ++i) {
      free(command.args[i].value);
    }

    free(command.args);
  }
}
