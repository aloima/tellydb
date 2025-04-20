#include "../../headers/telly.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

static struct Command *commands;
static uint32_t command_count;

struct Command *load_commands() {
  const struct Command _commands[] = {
    // Data commands
    cmd_bgsave,
    cmd_dbsize,
    cmd_lastsave,
    cmd_save,
    cmd_select,

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
    cmd_hgetall,
    cmd_hkeys,
    cmd_hlen,
    cmd_hset,
    cmd_htype,
    cmd_hvals,

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

  command_count = (sizeof(_commands) / sizeof(struct Command));

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
  struct Command *command = transaction->command;
  struct Client *client = transaction->client;
  struct Password *password = transaction->password;

  struct CommandEntry entry = {
    .client = client,
    .data = transaction->data,
    .database = transaction->database,
    .password = password
  };

  if ((password->permissions & command->permissions) != command->permissions) {
    _write(client, "-No permissions to execute this command\r\n", 41);
    return;
  }

  command->run(entry);
}
