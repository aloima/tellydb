#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>

static uint8_t read_permissions_value(struct Client *client, const char *value) {
  uint8_t permissions = 0;
  char *cval = (char *) value;
  char c;

  while ((c = *cval) != '\0') {
    switch (c) {
      case 'r':
        permissions |= P_READ;
        break;

      case 'w':
        permissions |= P_WRITE;
        break;

      case 'c':
        permissions |= P_CLIENT;
        break;

      case 'o':
        permissions |= P_CONFIG;
        break;

      case 'a':
        permissions |= P_AUTH;
        break;

      case 's':
        permissions |= P_SERVER;
        break;

      default: {
        char buf[27];
        sprintf(buf, "-Invalid permission: '%c'\r\n", c);

        if (client) _write(client, buf, 26);
        return 0;
      }
    }

    cval += 1;
  }

  return permissions;
}

static void add_pwd(struct CommandEntry entry) {
  if (entry.data->arg_count != 3) {
    if (entry.client) WRONG_ARGUMENT_ERROR(entry.client, "PWD ADD", 7);
    return;
  }

  const uint8_t all = get_full_password()->permissions;
  const string_t data = entry.data->args[1];
  const char *value = entry.data->args[2].value;
  const uint8_t permissions = (streq(value, "all") ? all : read_permissions_value(entry.client, value));
  const uint8_t not_have = ~entry.password->permissions & permissions;

  if (not_have) {
    if (entry.client) _write(entry.client, "-Tried to give permissions your password do not have\r\n", 54);
    return;
  }

  if (where_password(data.value, data.len) == -1) {
    add_password(entry.client, data, permissions);
    if (entry.client) WRITE_OK(entry.client);
    return;
  }

  _write(entry.client, "-This password already exists\r\n", 31);
}

static void edit_pwd(struct CommandEntry entry) {
  if (entry.data->arg_count != 3) {
    if (entry.client) WRONG_ARGUMENT_ERROR(entry.client, "PWD EDIT", 8);
    return;
  }

  const string_t input = entry.data->args[1];
  const char *value = entry.data->args[2].value;

  struct Password *target = get_password(input.value, input.len);
  if (!target) {
    if (entry.client) _write(entry.client, "-This password does not exist\r\n", 31);
    return;
  }

  const uint8_t permissions = read_permissions_value(entry.client, value);
  const uint8_t not_have = ~entry.password->permissions & permissions;

  if (not_have) {
    if (entry.client) _write(entry.client, "-Tried to give permissions your password do not have\r\n", 54);
    return;
  }

  target->permissions = permissions;
  if (entry.client) WRITE_OK(entry.client);
}

static void remove_pwd(struct CommandEntry entry) {
  if (entry.data->arg_count != 2) {
    if (entry.client) WRONG_ARGUMENT_ERROR(entry.client, "PWD REMOVE", 10);
    return;
  }

  const string_t input = entry.data->args[1];

  if (remove_password(entry.client, input.value, input.len) && entry.client) WRITE_OK(entry.client);
  else if (entry.client) _write(entry.client, "-This password cannot be found\r\n", 32);
}

static void run(struct CommandEntry entry) {
  if (entry.data->arg_count == 0) {
    if (entry.client) WRONG_ARGUMENT_ERROR(entry.client, "PWD", 3);
    return;
  }

  const string_t subcommand_string = entry.data->args[0];
  char subcommand[subcommand_string.len + 1];
  to_uppercase(subcommand_string.value, subcommand);

  if (streq(subcommand, "ADD")) {
    add_pwd(entry);
  } else if (streq(subcommand, "EDIT")) {
    edit_pwd(entry);
  } else if (streq(subcommand, "REMOVE")) {
    remove_pwd(entry);
  } else if (streq(subcommand, "GENERATE")) {
    if (!entry.client) return;

    char value[33];
    generate_random_string(value, 32);

    char buf[41];
    sprintf(buf, "$32\r\n%s\r\n", value);

    _write(entry.client, buf, 39);
  }
}

static struct Subcommand subcommands[] = {
  (struct Subcommand) {
    .name = "ADD",
    .summary = "Adds a password.",
    .since = "0.1.7",
    .complexity = "O(N) where N is permissions length"
  },
  (struct Subcommand) {
    .name = "EDIT",
    .summary = "Edits a password permissions.",
    .since = "0.1.7",
    .complexity = "O(N) where N is permissions length"
  },
  (struct Subcommand) {
    .name = "REMOVE",
    .summary = "Removes a password.",
    .since = "0.1.7",
    .complexity = "O(1)"
  },
  (struct Subcommand) {
    .name = "GENERATE",
    .summary = "Generates a password value.",
    .since = "0.1.7",
    .complexity = "O(1)"
  }
};

const struct Command cmd_pwd = {
  .name = "PWD",
  .summary = "Allows to manage passwords.",
  .since = "0.1.7",
  .complexity = "O(1)",
  .permissions = P_AUTH,
  .subcommands = subcommands,
  .subcommand_count = 0,
  .run = run
};
