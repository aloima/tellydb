#include <telly.h>

#include <stdio.h>
#include <stdint.h>
#include <stdatomic.h>

#include <strings.h>

static char *buf = NULL;
static Arena *arena = NULL;

int initialize_read_buffers() {
  buf = malloc(RESP_BUF_SIZE);
  if (buf == NULL) return -1;

  arena = arena_create(INITIAL_RESP_ARENA_SIZE);
  if (arena == NULL) {
    free(buf);
    return -1;
  }

  return 1;
}

void free_read_buffers() {
  if (buf) free(buf);
  if (arena) arena_destroy(arena);
}

static inline void unknown_command(Client *client, string_t *name) {
  char ubuf[COMMAND_NAME_MAX_LENGTH + 22];
  const size_t nbytes = sprintf(ubuf, "-Unknown command '%s'\r\n", name->value);

  add_io_request(IOOP_WRITE, client, CREATE_STRING(ubuf, nbytes));
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

void read_command(Client *client) {
  int size = read_from_socket(client, buf, RESP_BUF_SIZE);

  if (size == 0) {
    add_io_request(IOOP_TERMINATE, client, EMPTY_STRING());
    return;
  }

  int at = 0;

  while (size != -1) {
    commanddata_t data;
    if (!get_command_data(arena, client, buf, &at, &size, &data)) continue;

    if (size == at) {
      if (size != RESP_BUF_SIZE) {
        size = -1;
      } else {
        size = read_from_socket(client, buf, RESP_BUF_SIZE);
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
