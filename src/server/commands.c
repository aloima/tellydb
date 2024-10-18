#include "../../headers/server.h"
#include "../../headers/commands.h"
#include "../../headers/utils.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

static struct Command *commands = NULL;
static uint32_t command_count = 24;

void load_commands() {
  struct Command _commands[] = {
    // Data commands
    cmd_decr,
    cmd_exists,
    cmd_get,
    cmd_incr,
    cmd_set,
    cmd_type,

    // Generic commands
    cmd_age,
    cmd_client,
    cmd_command,
    cmd_info,
    cmd_memory,
    cmd_ping,
    cmd_time,

    // Hashtable commands
    cmd_hdel,
    cmd_hget,
    cmd_hlen,
    cmd_hset,
    cmd_htype,

    // List commands
    cmd_lindex,
    cmd_llen,
    cmd_lpop,
    cmd_lpush,
    cmd_rpop,
    cmd_rpush
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

void execute_command(struct Client *client, respdata_t *data) {
  if (data->type == RDT_ARRAY) {
    const string_t name = data->value.array[0]->value.string;

    char input[name.len + 1];
    to_uppercase(name.value, input);

    bool executed = false;

    for (uint32_t i = 0; i < command_count; ++i) {
      const struct Command command = commands[i];

      if (streq(input, command.name)) {
        command.run(client, data);
        executed = true;
        break;
      }
    }

    if (!executed && client != NULL) {
      const uint32_t len = 21 + data->value.array[0]->value.string.len;
      char res[len + 1];
      sprintf(res, "-unknown command '%s'\r\n", input);

      _write(client, res, len);
    }
  } else {
    write_log(LOG_ERR, "Received data from Client #%d is not RDT_ARRAY, so it is not readable as a command.", client->id);
  }
}
