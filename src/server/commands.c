#include "../../headers/telly.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <pthread.h>
#include <unistd.h>

static struct Command *commands = NULL;
static uint32_t command_count = 3;

void load_commands() {
  struct Command scommands[] = {cmd_command, cmd_get, cmd_info};
  commands = malloc(sizeof(scommands));
  memcpy(commands, scommands, sizeof(scommands));
}

struct Command *get_commands() {
  return commands;
}

uint32_t get_command_count() {
  return command_count;
}

void execute_command(struct Client *client, respdata_t *data, struct Configuration conf) {
  if (data->type == RDT_ARRAY) {
    char *input = data->value.array[0].value.string.value;

    for (uint32_t i = 0; i < command_count; ++i) {
      struct Command command = commands[i];

      if (streq(input, command.name)) {
        command.run(client, data, conf);
        break;
      }
    }
  } else {
    client_error();
  }
}
