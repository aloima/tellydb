#include "../../headers/server.h"
#include "../../headers/commands.h"
#include "../../headers/database.h"
#include "../../headers/utils.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static struct Command *commands = NULL;
static uint32_t command_count = 31;

void load_commands() {
  struct Command _commands[] = {
    // Data commands
    cmd_bgsave,
    cmd_dbsize,
    cmd_lastsave,
    cmd_save,

    // Generic commands
    cmd_age,
    cmd_auth,
    cmd_client,
    cmd_command,
    cmd_hello,
    cmd_info,
    cmd_ping,
    cmd_pwd,
    cmd_time,

    // Hashtable commands
    cmd_hdel,
    cmd_hget,
    cmd_hlen,
    cmd_hset,
    cmd_htype,

    // KV commands
    cmd_append,
    cmd_decr,
    cmd_exists,
    cmd_get,
    cmd_incr,
    cmd_set,
    cmd_type,

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

void execute_command(struct Transaction *transaction) {
  commanddata_t *data = transaction->command;

  char input[data->name.len + 1];
  to_uppercase(data->name.value, input);

  for (uint32_t i = 0; i < command_count; ++i) {
    const struct Command command = commands[i];

    if (streq(input, command.name)) {
      command.run(transaction->client, transaction->command, transaction->password);
      return;
    }
  }

  if (transaction->client) {
    char buf[42];
    const size_t nbytes = sprintf(buf, "-Unknown command '%s'\r\n", input);

    _write(transaction->client, buf, nbytes);
  }
}
