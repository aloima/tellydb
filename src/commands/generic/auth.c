#include <telly.h>

#include <stddef.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->data->arg_count != 1 && entry->data->arg_count != 2) {
    return WRONG_ARGUMENT_ERROR("AUTH");
  }

  const string_t input = entry->data->args[0];
  const int target = where_password(input.value, input.len);

  if (target == -1) {
    return RESP_ERROR_MESSAGE("This password does not exist");
  }

  if (entry->password && entry->password != get_empty_password()) {
    if (entry->data->arg_count != 2) {
      return RESP_OK_MESSAGE("A password already in use for your client. If you sure to change, use command with ok argument");
    }

    const char *ok = entry->data->args[1].value;

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
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
