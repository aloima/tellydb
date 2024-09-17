#include "../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <pthread.h>
#include <unistd.h>

static struct Command *commands = NULL;
static uint32_t command_count = 15;

void load_commands() {
  struct Command _commands[] = {
    cmd_client,
    cmd_command,
    cmd_decr,
    cmd_get,
    cmd_hget,
    cmd_hlen,
    cmd_hset,
    cmd_htype,
    cmd_incr,
    cmd_info,
    cmd_memory,
    cmd_ping,
    cmd_set,
    cmd_time,
    cmd_type
  };

  commands = malloc(sizeof(_commands));
  memcpy(commands, _commands, sizeof(_commands));
}

void free_commands() {
  free(commands);
}

struct Command *get_commands() {
  return commands;
}

uint32_t get_command_count() {
  return command_count;
}

void execute_command(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (data->type == RDT_ARRAY) {
    string_t name = data->value.array[0]->value.string;

    char input[name.len + 1];
    to_uppercase(name.value, input);

    bool executed = false;

    for (uint32_t i = 0; i < command_count; ++i) {
      struct Command command = commands[i];

      if (streq(input, command.name)) {
        command.run(client, data, conf);
        executed = true;
        break;
      }
    }

    if (!executed && client != NULL) {
      const uint32_t len = 21 + data->value.array[0]->value.string.len;
      char res[len + 1];
      sprintf(res, "-unknown command '%s'\r\n", input);

      write(client->connfd, res, len);
    }
  } else {
    client_error();
  }
}
