#include <telly.h>
#include "io.h"

#include <stdio.h>
#include <stdint.h>
#include <stdatomic.h>

#include <strings.h>

static inline void unknown_command(Client *client, string_t *name) {
  char buf[COMMAND_NAME_MAX_LENGTH + 22];
  const size_t nbytes = sprintf(buf, "-Unknown command '%s'\r\n", name->value);

  _write(client, buf, nbytes);
}

static inline void get_used_command(const commanddata_t *data, UsedCommand *command) {
  const struct CommandIndex *command_index = get_command_index(data->name->value, data->name->len);
  if (!command_index) {
    atomic_store_explicit(&command->idx, UINT64_MAX, memory_order_relaxed);
    atomic_store_explicit(&command->data, NULL, memory_order_relaxed);
    atomic_store_explicit(&command->subcommand, NULL, memory_order_relaxed);

    return;
  }

  struct Command *command_data = &server->commands[command_index->idx];
  struct Subcommand *subcommand = NULL;

  if (data->args.count != 0) {
    struct Subcommand *subcommands = command_data->subcommands;
    const uint32_t subcommand_count = command_data->subcommand_count;
    const char *value = data->args.data[0].value;

    for (uint32_t i = 0; i < subcommand_count; ++i) {
      if (strcasecmp(subcommands[i].name, value) == 0) {
        subcommand = &subcommands[i];
        break;
      }
    }
  }

  atomic_store_explicit(&command->idx, command_index->idx, memory_order_relaxed);
  atomic_store_explicit(&command->data, command_data, memory_order_relaxed);
  atomic_store_explicit(&command->subcommand, subcommand, memory_order_relaxed);
}

void read_command(IOThread *thread, Client *client) {
  char *buf = thread->read_buf;
  __builtin_prefetch(buf, 0, 3);
  __builtin_prefetch(buf, 1, 3);

  int32_t size = _read(client, buf, RESP_BUF_SIZE);

  if (size == 0) {
    add_io_request(IOOP_TERMINATE, client, EMPTY_STRING());
    return;
  }

  Arena *arena = thread->arena;
  __builtin_prefetch(arena, 0, 3);
  __builtin_prefetch(arena, 1, 3);

  int32_t at = 0;

  while (size != -1) {
    commanddata_t data;
    if (!get_command_data(arena, client, buf, &at, &size, &data)) continue;

    if (size == at) {
      if (size != RESP_BUF_SIZE) {
        size = -1;
      } else {
        size = _read(client, buf, RESP_BUF_SIZE);
        at = 0;
      }
    }

    if (client->locked) {
      WRITE_ERROR_MESSAGE(client, "Your client is locked, you cannot use any commands until your client is unlocked");
      continue;
    }

    const struct CommandIndex *command_index = get_command_index(data.name->value, data.name->len);

    if (!command_index) {
      unknown_command(client, data.name);
      continue;
    }

    UsedCommand *command = client->command;
    get_used_command(&data, client->command);

    if (!add_transaction(client, client->command, &data)) {
      WRITE_ERROR_MESSAGE(client, "Transaction cannot be enqueued because of server settings");
      write_log(LOG_WARN, "Transaction count reached their limit, so next transactions cannot be added.");
      return;
    }

    if (client->waiting_block && !server->commands[command->idx].flags.bits.waiting_tx) {
      WRITE_OK_MESSAGE(client, "QUEUED");
      continue;
    }
  }
}
