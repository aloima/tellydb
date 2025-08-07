#include <telly.h>

#include <stddef.h>

static void run(struct CommandEntry entry) {
  if (!entry.client) return;
  if (entry.data->arg_count != 1 && entry.data->arg_count != 2) {
    WRONG_ARGUMENT_ERROR(entry.client, "AUTH");
    return;
  }

  const string_t input = entry.data->args[0];
  struct Password *found = get_password(input.value, input.len);

  if (!found) {
    WRITE_ERROR_MESSAGE(entry.client, "This password does not exist");
    return;
  }

  if (entry.password && entry.password != get_empty_password()) {
    if (entry.data->arg_count != 2) {
      _write(entry.client, "+A password already in use for your client. If you sure to change, use command with ok argument\r\n", 97);
      return;
    }

    const char *ok = entry.data->args[1].value;

    if (!streq(ok, "ok")) {
      _write(entry.client, "+A password already in use for your client. If you sure to change, use command with ok argument\r\n", 97);
      return;
    }
  }

  entry.client->password = found;
  WRITE_OK(entry.client);
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
