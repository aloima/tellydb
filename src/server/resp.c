#include "../../headers/server.h"
#include "../../headers/utils.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#define DATA_ERR(client) write_log(LOG_ERR, "Received data from Client #%d cannot be validated as a RESP data, so it cannot be created as a command.", (client)->id);

static string_t parse_resp_bstring(struct Client *client) {
  string_t data = {
    .len = 0,
    .value = NULL
  };

  char c;
  _read(client, &c, 1);

  if (c == RDT_BSTRING) {
    while (true) {
      _read(client, &c, 1);

      if (isdigit(c)) {
        data.len = (data.len * 10) + (c - 48);
      } else if (c == '\r') {
        _read(client, &c, 1);

        if (c == '\n') {
          data.value = malloc(data.len + 1);
          uint64_t total = data.len;

          while (total != 0) {
            total -= _read(client, data.value + data.len - total, total);
          }

          data.value[data.len] = '\0';

          char buf[2];
          _read(client, buf, 2);

          if (buf[0] != '\r' || buf[1] != '\n') {
            DATA_ERR(client);
            free(data.value);
            return (string_t) {0};
          }

          return data;
        } else {
          DATA_ERR(client);
          return (string_t) {0};
        }
      } else {
        DATA_ERR(client);
        return (string_t) {0};
      }
    }
  } else {
    DATA_ERR(client);
    return (string_t) {0};
  }

  return data;
}

static commanddata_t *parse_resp_command(struct Client *client) {
  commanddata_t *command = malloc(sizeof(commanddata_t));
  command->arg_count = 0;
  command->args = NULL;

  while (true) {
    char c;
    _read(client, &c, 1);

    if (isdigit(c)) {
      command->arg_count = (command->arg_count * 10) + (c - 48);
    } else if (c == '\r') {
      _read(client, &c, 1);

      if (c == '\n') {
        if (command->arg_count == 0) {
          _write(client, "Received data from Client #%d is empty RESP data, so it cannot be created as a command.", client->id);
          free(command);
          return NULL;
        } else {
          command->arg_count -= 1;
          command->name = parse_resp_bstring(client);

          if (command->arg_count != 0) {
            command->args = malloc(command->arg_count * sizeof(string_t));
            command->args[0] = parse_resp_bstring(client);

            for (uint32_t i = 1; i < command->arg_count; ++i) {
              command->args[i] = parse_resp_bstring(client);
            }
          }

          return command;
        }
      } else {
        DATA_ERR(client);
        free(command);
        return NULL;
      }
    } else {
      DATA_ERR(client);
      free(command);
      return NULL;
    }
  }
}

commanddata_t *get_command_data(struct Client *client) {
  uint8_t type;

  if (_read(client, &type, 1) == 0) {
    commanddata_t *command = malloc(sizeof(commanddata_t));
    command->close = true;

    return command;
  } else if (type == RDT_ARRAY) {
    commanddata_t *command = parse_resp_command(client);
    command->close = false;

    return command;
  }

  return NULL;
}

void free_command_data(commanddata_t *command) {
  if (command->arg_count != 0) {
    free(command->args[0].value);

    for (uint32_t i = 1; i < command->arg_count; ++i) {
      free(command->args[i].value);
    }

    free(command->args);
  }

  free(command->name.value);
  free(command);
}
