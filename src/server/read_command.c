#include <telly.h>

static inline void unknown_command(Client *client, string_t *name, Arena *arena) {
  char *ubuf = arena_alloc(arena, name->len + 22);
  const size_t nbytes = sprintf(ubuf, "-Unknown command '%.*s'\r\n", name->len, name->value);

  add_io_request(IOOP_WRITE, client, CREATE_STRING(ubuf, nbytes));
}

static inline void get_used_command(const commanddata_t *data, UsedCommand *command) {
  const struct CommandIndex *command_index = get_command_index(data->name.value, data->name.len);
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
  if (atomic_load_explicit(&client->read_buf->refcount, memory_order_relaxed) > 1) {
    QueryBuffer *new_buf = malloc(sizeof(QueryBuffer));
    new_buf->data = malloc(RESP_BUF_SIZE);
    new_buf->size = RESP_BUF_SIZE;
    atomic_init(&new_buf->refcount, 1);

    atomic_fetch_sub_explicit(&client->read_buf->refcount, 1, memory_order_relaxed);
    client->read_buf = new_buf;
  }

  int size = read_from_socket(client, client->read_buf->data, client->read_buf->size);

  if (size == 0) {
    add_io_request(IOOP_TERMINATE, client, EMPTY_STRING());
    return;
  }

  int at = 0;

  while (size != -1) {
    commanddata_t data;
    if (!get_command_data(client, &at, &size, &data))
      continue;

    if (size == at) {
      // TODO: safe type-casting
      if (size != (int32_t) client->read_buf->size) {
        size = -1;
      } else {
        if (atomic_load_explicit(&client->read_buf->refcount, memory_order_relaxed) > 1) {
          QueryBuffer *new_buf = malloc(sizeof(QueryBuffer));
          new_buf->data = malloc(RESP_BUF_SIZE);
          new_buf->size = RESP_BUF_SIZE;
          atomic_init(&new_buf->refcount, 1);

          atomic_fetch_sub_explicit(&client->read_buf->refcount, 1, memory_order_relaxed);
          client->read_buf = new_buf;
        }

        size = read_from_socket(client, client->read_buf->data, client->read_buf->size);
        at = 0;
      }
    }

    if (client->locked) {
      WRITE_ERROR_MESSAGE(client, "Your client is locked, you cannot use any commands until your client is unlocked");
      continue;
    }

    const struct CommandIndex *command_index = get_command_index(data.name.value, data.name.len);

    if (!command_index) {
      unknown_command(client, &data.name, thread->ucmd_arena);
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
