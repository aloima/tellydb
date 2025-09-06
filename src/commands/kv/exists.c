#include <telly.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

static string_t run(struct CommandEntry entry) {
  PASS_NO_CLIENT(entry.client);

  if (entry.data->arg_count == 0) {
    return WRONG_ARGUMENT_ERROR("EXISTS");
  }

  uint32_t existed = 0, not_existed = 0;

  char buf[8192];
  buf[0] = '\0';

  for (uint32_t i = 0; i < entry.data->arg_count; ++i) {
    const string_t key = entry.data->args[i];

    if (get_data(entry.database, key)) {
      existed += 1;
      strcat(buf, "+exists\r\n");
    } else {
      not_existed += 1;
      strcat(buf, "+not exist\r\n");
    }
  }

  // calculated length: 85 + (existed * 9) + (not_existed * 12)
  const size_t nbytes = sprintf(entry.buffer, (
    "*%u\r\n"
      "+existed key count is %u\r\n"
      "+not existed key count is %u\r\n"
      "%s"
  ), entry.data->arg_count + 2, existed, not_existed, buf);

  return CREATE_STRING(entry.buffer, nbytes);
}

const struct Command cmd_exists = {
  .name = "EXISTS",
  .summary = "Checks if specified keys exist or not.",
  .since = "0.1.4",
  .complexity = "O(N) where N is key count",
  .permissions = P_NONE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
