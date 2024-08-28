#include "../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <pthread.h>
#include <unistd.h>

static struct Command *commands = NULL;
static uint32_t command_count = 8;

void load_commands() {
  struct Command scommands[] = {cmd_client, cmd_command, cmd_decr, cmd_get, cmd_incr, cmd_info, cmd_set, cmd_type};
  commands = malloc(sizeof(scommands));
  memcpy(commands, scommands, sizeof(scommands));
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
    char *input = data->value.array[0].value.string.value;
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
      const uint32_t len = 21 + data->value.array[0].value.string.len;
      char res[len + 1];
      sprintf(res, "-unknown command '%s'\r\n", input);

      write(client->connfd, res, len);
    }
  } else {
    client_error();
  }
}
