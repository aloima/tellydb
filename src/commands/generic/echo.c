#include <telly.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  switch (entry->args->count) {
    case 1: {
      const size_t nbytes = create_resp_string(entry->client->write_buf, entry->args->data[0]);
      return CREATE_STRING(entry->client->write_buf, nbytes);
    }

    default:
      return WRONG_ARGUMENT_ERROR("ECHO");
  }
}

const struct Command cmd_echo = {
  .name = "ECHO",
  .summary = "Returns the given message.",
  .since = "1.0.0",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .flags.value = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
