#include "../../headers/telly.h"

#include <stdint.h>
#include <stdlib.h>

static struct Command *commands = NULL;
static uint32_t command_count = 2;

void load_commands() {
  struct Command scommands[] = {cmd_command, cmd_get};
  commands = malloc(sizeof(scommands));
  memcpy(commands, scommands, sizeof(scommands));
}

struct Command *get_commands() {
  return commands;
}

uint32_t get_command_count() {
  return command_count;
}

void execute_commands(int connfd, respdata_t data) {
  if (data.type == RDT_ARRAY) {
    char *input = data.value.array[0].value.string.data;

    for (uint32_t i = 0; i < command_count; ++i) {
      struct Command command = commands[i];

      if (streq(input, command.name)) {
        command.run(connfd, data);
        break;
      }
    }
  } else {
    client_error();
  }
}
