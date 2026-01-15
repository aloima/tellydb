#include <telly.h>

#include <stddef.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 1 && entry->args->count != 2) {
    return WRONG_ARGUMENT_ERROR("AUTH");
  }

  const string_t input = entry->args->data[0];
  const int target = where_password(input.value, input.len);

  if (target == -1) {
    return RESP_ERROR_MESSAGE("This password does not exist");
  }

  if (entry->password && entry->password != get_empty_password()) {
    if (entry->args->count != 2) {
      return RESP_OK_MESSAGE("A password already in use for your client. If you sure to change, use command with ok argument");
    }

    const char *ok = entry->args->data[1].value;

    if (!streq(ok, "ok")) {
      return RESP_OK_MESSAGE("A password already in use for your client. If you sure to change, use command with ok argument");
    }
  }

  struct Password **passwords = get_passwords();
  entry->client->password = passwords[target];
  return RESP_OK();
}

const struct Command cmd_auth = {
  .name = "AUTH",
  .summary = "Allows to authorize client via passwords.",
  .since = "0.1.7",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .flags.value = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
