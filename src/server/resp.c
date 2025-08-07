#include <telly.h>

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#define DATA_ERR(client) \
  write_log(LOG_ERR, "Received data from Client #%u cannot be validated as a RESP data, so it cannot be created as a command.", (client)->id);

#define TAKE_BYTES(value, n, return_value) \
  if (take_n_bytes(client, buf, at, value, n, size) != n) { \
    DATA_ERR(client); \
    return return_value; \
  }

static int take_n_bytes(struct Client *client, char *buf, int32_t *at, void *data, const uint32_t n, int32_t *size) {
  if (VERY_LIKELY((*at + n) < RESP_BUF_SIZE)) {
    memcpy(data, buf + *at, n);
    *at += n;

    return n;
  } else {
    const uint32_t remaining = (RESP_BUF_SIZE - *at);
    memcpy(data, buf + *at, remaining);

    if (_read(client, data + remaining, n - remaining) <= 0) {
      return remaining;
    }

    *at = 0;
    *size = 0;
    return n;
  }
}

static string_t parse_resp_bstring(struct Client *client, char *buf, int32_t *at, int32_t *size) {
  string_t data = {
    .len = 0,
    .value = NULL
  };

  char c;
  TAKE_BYTES(&c, 1, ((string_t) {0}));

  if (VERY_UNLIKELY(c != RDT_BSTRING)) {
    DATA_ERR(client);
    return (string_t) {0};
  }

  while (true) {
    TAKE_BYTES(&c, 1, ((string_t) {0}));

    if (isdigit(c)) {
      data.len = (data.len * 10) + (c - 48);
    } else if (c == '\r') {
      TAKE_BYTES(&c, 1, ((string_t) {0}));

      if (c != '\n') {
        DATA_ERR(client);
        return (string_t) {0};
      }

      data.value = malloc(data.len + 1);
      TAKE_BYTES(data.value, data.len, ((string_t) {0}));
      data.value[data.len] = '\0';

      char crlf[2];
      TAKE_BYTES(crlf, 2, ((string_t) {0}));

      if (VERY_UNLIKELY(crlf[0] != '\r' || crlf[1] != '\n')) {
        DATA_ERR(client);
        free(data.value);
        return (string_t) {0};
      }

      return data;
    } else {
      DATA_ERR(client);
      return (string_t) {0};
    }
  }

  return data;
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

      command->name = parse_resp_bstring(client, buf, at, size);
      command->arg_count -= 1;

      if (command->arg_count != 0) {
        command->args = malloc(command->arg_count * sizeof(string_t));

        for (uint32_t i = 0; i < command->arg_count; ++i) {
          command->args[i] = parse_resp_bstring(client, buf, at, size);
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

  free(command.name.value);
}
