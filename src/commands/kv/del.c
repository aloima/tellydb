#include <telly.h>

#include <stdio.h>
#include <stdint.h>

static string_t run(struct CommandEntry *entry) {
  if (entry->data->arg_count == 0) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("DEL");
  }

  uint32_t deleted = 0;

  for (uint32_t i = 0; i < entry->data->arg_count; ++i) {
    deleted += delete_data(entry->database, entry->data->args[i]);
  }

  PASS_NO_CLIENT(entry->client);
  const size_t res_len = create_resp_integer(entry->buffer, deleted);
  return CREATE_STRING(entry->buffer, res_len);
}

const struct Command cmd_del = {
  .name = "DEL",
  .summary = "Deletes the specified keys.",
  .since = "0.1.7",
  .complexity = "O(N) where N is key count",
  .permissions = P_WRITE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
