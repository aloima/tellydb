#include "../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

static struct Command *commands = NULL;
static uint32_t command_count = 33;

struct Command *load_commands() {
  const struct Command _commands[] = {
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
    cmd_del,
    cmd_exists,
    cmd_get,
    cmd_incr,
    cmd_rename,
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

  if (posix_memalign((void **) &commands, 8, sizeof(_commands)) == 0) {
    memcpy(commands, _commands, sizeof(_commands));
  } else {
    write_log(LOG_ERR, "Cannot create commands, out of memory.");
  }

  return commands;
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
  if (transaction->command) {
    commanddata_t *data = transaction->data;
    struct Command command = *transaction->command;

    command.run(transaction->client, data, transaction->password);
  } else if (transaction->client) {
    char buf[42];
    const size_t nbytes = sprintf(buf, "-Unknown command '%s'\r\n", transaction->data->name.value);

    _write(transaction->client, buf, nbytes);
  }
}
