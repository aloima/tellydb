#include <telly.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>

static inline void DATA_ERR(struct Client *client) {
  write_log(LOG_ERR, "Received data from Client #%" PRIu32 " cannot be validated as a RESP data, so it cannot be created as a command.", client->id);
}

#define TAKE_BYTES(value, n, return_value) \
  if (take_n_bytes(client, buf, at, value, n, size) != n) { \
    DATA_ERR(client); \
    return return_value; \
  }

static inline int take_n_bytes(struct Client *client, char *buf, int32_t *at, void *data, const uint32_t n, int32_t *size) {
  const uint32_t remaining = (RESP_BUF_SIZE - *at);

  if (VERY_LIKELY(n < remaining)) {
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

    // Needs to read buffer, because the process may be interrupted in the middle of getting command data
    *size = _read(client, buf, RESP_BUF_SIZE);

    if (*size == -1) {
      return -1;
    }
  } else {
    char dummy[128];
    const uint32_t length = (n - remaining);
    const uint32_t quotient = (length / sizeof(dummy));
    const uint32_t rest = (length % sizeof(dummy));
    uint32_t total = remaining;

    for (uint32_t i = 0; i < quotient; ++i) {
      int got = _read(client, dummy, sizeof(dummy));

      if (got <= 0) {
        return total + (got > 0 ? got : 0);
      }

      total += got;

      if ((uint32_t) got < sizeof(dummy)) {
        return total;
      }
    }

    if (rest > 0) {
      int got = _read(client, dummy, rest);

      if (got <= 0) {
        return total + (got > 0 ? got : 0);
      }

      total += got;

      if ((uint32_t) got < rest) {
        return total;
      }
    }

    *size = _read(client, buf, RESP_BUF_SIZE);

    if (*size <= 0) {
      return total;
    }
  }

  *at = 0;
  return n;
}

static inline bool get_resp_command_name(struct Client *client, commandname_t *name, char *buf, int32_t *at, int32_t *size) {
  char c;
  TAKE_BYTES(&c, 1, false);

  if (VERY_UNLIKELY(c != RDT_BSTRING)) {
    DATA_ERR(client);
    return false;
  }

  while (true) {
    TAKE_BYTES(&c, 1, false);

    if (isdigit(c)) {
      name->len = (name->len * 10) + (c - 48);
    } else if (c == '\r') {
      TAKE_BYTES(&c, 1, false);

      if (c != '\n') {
        DATA_ERR(client);
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

      char crlf[2];
      TAKE_BYTES(crlf, 2, false);

      if (VERY_UNLIKELY(crlf[0] != '\r' || crlf[1] != '\n')) {
        DATA_ERR(client);
        return false;
      }

      return true;
    } else {
      DATA_ERR(client);
      return false;
    }
  }
}

static inline bool get_resp_command_argument(struct Client *client, string_t *argument, char *buf, int32_t *at, int32_t *size) {
  char c;
  TAKE_BYTES(&c, 1, false);

  if (VERY_UNLIKELY(c != RDT_BSTRING)) {
    DATA_ERR(client);
    return false;
  }

  while (true) {
    TAKE_BYTES(&c, 1, false);

    if (isdigit(c)) {
      argument->len = (argument->len * 10) + (c - 48);
    } else if (c == '\r') {
      TAKE_BYTES(&c, 1, false);

      if (c != '\n') {
        DATA_ERR(client);
        return false;
      }

      argument->value = malloc(argument->len + 1);

      if (!argument->value) {
        DATA_ERR(client);
        return false;
      }

      TAKE_BYTES(argument->value, argument->len, false);
      argument->value[argument->len] = '\0';

      char crlf[2] = {0, 0};
      TAKE_BYTES(crlf, 2, false);

      if (VERY_UNLIKELY(crlf[0] != '\r' || crlf[1] != '\n')) {
        DATA_ERR(client);
        return false;
      }

      return true;
    } else {
      DATA_ERR(client);
      return false;
    }
  }
}

static bool parse_resp_command(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command) {
  command->arg_count = 0;
  command->args = NULL;

  while (true) {
    char c;
    TAKE_BYTES(&c, 1, false);

    if (isdigit(c)) {
      command->arg_count = (command->arg_count * 10) + (c - 48);
    } else if (c == '\r') {
      TAKE_BYTES(&c, 1, false);

      if (c != '\n') {
        DATA_ERR(client);
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

          if (!get_resp_command_argument(client, &command->args[i], buf, at, size)) {
            for (uint32_t j = 0; j < i; ++j) {
              free(command->args[j].value);
            }

            free(command->args);
            return false;
          }
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
  TAKE_BYTES(&type, 1, false);

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
