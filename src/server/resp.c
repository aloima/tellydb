#include "../../headers/telly.h"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#define DATA_ERR(client) write_log(LOG_ERR, "Received data from Client #%d cannot be validated as a RESP data, so it cannot be created as a command.", (client)->id);

static int take_n_bytes(struct Client *client, char *buf, int32_t *at, void *data, const uint32_t n, int32_t *size) {
  uint64_t count = 0;

  if (VERY_LIKELY((*at + n) < RESP_BUF_SIZE)) {
    memcpy(data, buf + *at, n);
    *at += n;
  } else {
    const uint32_t remaining = (RESP_BUF_SIZE - *at);

    memcpy(data, buf + *at, remaining);
    _read(client, data + remaining, n - remaining);

    *at = 0;
    *size = 0;
  }

  return count;
}

static string_t parse_resp_bstring(struct Client *client, char *buf, int32_t *at, int32_t *size) {
  string_t data = {
    .len = 0,
    .value = NULL
  };

  char c;
  take_n_bytes(client, buf, at, &c, 1, size);

  if (VERY_UNLIKELY(c != RDT_BSTRING)) {
    DATA_ERR(client);
    return (string_t) {0};
  }

  while (true) {
    take_n_bytes(client, buf, at, &c, 1, size);

    if (isdigit(c)) {
      data.len = (data.len * 10) + (c - 48);
    } else if (c == '\r') {
      take_n_bytes(client, buf, at, &c, 1, size);

      if (c != '\n') {
        DATA_ERR(client);
        return (string_t) {0};
      }

      data.value = malloc(data.len + 1);
      take_n_bytes(client, buf, at, data.value, data.len, size);
      data.value[data.len] = '\0';

      char crlf[2];
      take_n_bytes(client, buf, at, crlf, 2, size);

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

static commanddata_t *parse_resp_command(struct Client *client, char *buf, int32_t *at, int32_t *size) {
  commanddata_t *command = malloc(sizeof(commanddata_t));
  command->arg_count = 0;
  command->args = NULL;

  while (true) {
    char c;
    take_n_bytes(client, buf, at, &c, 1, size);

    if (isdigit(c)) {
      command->arg_count = (command->arg_count * 10) + (c - 48);
    } else if (c == '\r') {
      take_n_bytes(client, buf, at, &c, 1, size);

      if (c != '\n') {
        DATA_ERR(client);
        free(command);
        return NULL;
      }

      if (command->arg_count == 0) {
        write_log(LOG_ERR, "Received data from Client #%d is empty RESP data, so it cannot be created as a command.", client->id);
        free(command);
        return NULL;
      }

      command->name = parse_resp_bstring(client, buf, at, size);
      command->arg_count -= 1;

      if (command->arg_count != 0) {
        command->args = malloc(command->arg_count * sizeof(string_t));

        for (uint32_t i = 0; i < command->arg_count; ++i) {
          command->args[i] = parse_resp_bstring(client, buf, at, size);
        }
      }

      return command;
    } else {
      DATA_ERR(client);
      free(command);
      return NULL;
    }
  }
}

commanddata_t *get_command_data(struct Client *client, char *buf, int32_t *at, int32_t *size) {
  uint8_t type;
  take_n_bytes(client, buf, at, &type, 1, size);

  if (VERY_LIKELY(type == RDT_ARRAY)) return parse_resp_command(client, buf, at, size);
  return NULL;
}

void free_command_data(commanddata_t *command) {
  if (command->arg_count != 0) {
    for (uint32_t i = 0; i < command->arg_count; ++i) free(command->args[i].value);
    free(command->args);
  }

  free(command->name.value);
  free(command);
}
