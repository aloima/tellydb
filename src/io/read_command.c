#include <telly.h>
#include "io.h"

#include <stdio.h>
#include <stdint.h>

static inline void unknown_command(Client *client, string_t *name) {
  char buf[COMMAND_NAME_MAX_LENGTH + 22];
  const size_t nbytes = sprintf(buf, "-Unknown command '%s'\r\n", name->value);

  _write(client, buf, nbytes);
}

void read_command(IOThread *thread, Client *client) {
  char *buf = thread->read_buf;
  int32_t size = _read(client, buf, RESP_BUF_SIZE);

  if (size == 0) {
    add_io_request(IOOP_TERMINATE, client, EMPTY_STRING());
    return;
  }

  Arena *arena = thread->arena;
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
      return;
    }

    const uint64_t command_idx = command_index->idx;
    // client->command = &commands[command_idx];

    if (!add_transaction(client, command_idx, &data)) {
      WRITE_ERROR_MESSAGE(client, "Transaction cannot be enqueued because of server settings");
      write_log(LOG_WARN, "Transaction count reached their limit, so next transactions cannot be added.");
      return;
    }

    if (client->waiting_block && !IS_RELATED_TO_WAITING_TX(commands, command_idx)) _write(client, "+QUEUED\r\n", 9);
  }
}
