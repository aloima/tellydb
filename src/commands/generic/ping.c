#include <telly.h>

#include <stddef.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  switch (entry->data->arg_count) {
    case 0:
      return RESP_OK_MESSAGE("PONG");

    case 1: {
      const size_t nbytes = create_resp_string(entry->buffer, entry->data->args[0]);
      return CREATE_STRING(entry->buffer, nbytes);
    }

    default:
      return WRONG_ARGUMENT_ERROR("PING");
  }
}

const struct Command cmd_ping = {
  .name = "PING",
  .summary = "Pings the server and returns a simple/bulk string.",
  .since = "0.1.2",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
