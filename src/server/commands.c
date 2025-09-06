#include <telly.h>

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

static struct Command *commands;
static uint32_t command_count;

struct Command *load_commands() {
  const struct Command command_list[] = {
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
    cmd_discard,
    cmd_exec,
    cmd_hello,
    cmd_info,
    cmd_multi,
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

  command_count = (sizeof(command_list) / sizeof(struct Command));

  if (posix_memalign((void **) &commands, 32, sizeof(command_list)) == 0) {
    for (int i = 0; i < command_count; ++i) {
      struct Command command = command_list[i];
      uint32_t index = get_command_index(command.name, strlen(command.name))->idx;

      commands[index] = command;
    }
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

