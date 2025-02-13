#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>

static uint8_t read_permissions_value(struct Client *client, char *permissions_value) {
  uint8_t permissions = 0;
  char c;

  while ((c = *permissions_value) != '\0') {
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

        _write(client, buf, 26);
        return 0;
      }
    }

    permissions_value += 1;
  }

  return permissions;
}

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (command->arg_count == 0) {
    WRONG_ARGUMENT_ERROR(client, "PWD", 3);
    return;
  }

  if (password->permissions & P_AUTH) {
    const string_t subcommand_string = command->args[0];
    char subcommand[subcommand_string.len + 1];
    to_uppercase(subcommand_string.value, subcommand);

    if (streq(subcommand, "ADD")) {
      if (command->arg_count != 3) {
        WRONG_ARGUMENT_ERROR(client, "PWD ADD", 7);
        return;
      }

      const uint8_t all = get_full_password()->permissions;
      const string_t data = command->args[1];
      char *permissions_value = command->args[2].value;
      const uint8_t permissions = (streq(permissions_value, "all") ? all : read_permissions_value(client, permissions_value));
      const uint8_t not_has = ~password->permissions & permissions;

      if (not_has) {
        _write(client, "-Tried to give permissions your password do not have\r\n", 54);
        return;
      }

      if (where_password(data.value, data.len) == -1) {
        add_password(client, data, permissions);
        WRITE_OK(client);
      } else {
        _write(client, "-This password already exists\r\n", 31);
      }
    } if (streq(subcommand, "EDIT")) {
      if (command->arg_count != 3) {
        WRONG_ARGUMENT_ERROR(client, "PWD EDIT", 8);
        return;
      }

      const string_t input = command->args[1];
      char *permissions_value = command->args[2].value;

      struct Password *target = get_password(input.value, input.len);

      if (target) {
        const uint8_t permissions = read_permissions_value(client, permissions_value);
        const uint8_t not_has = ~password->permissions & permissions;

        if (not_has) {
          _write(client, "-Tried to give permissions your password do not have\r\n", 54);
          return;
        }

        target->permissions = permissions;
        WRITE_OK(client);
      } else {
        _write(client, "-This password does not exist\r\n", 31);
      }
    } else if (streq(subcommand, "REMOVE")) {
      if (command->arg_count != 2) {
        WRONG_ARGUMENT_ERROR(client, "PWD REMOVE", 10);
        return;
      }

      const string_t input = command->args[1];

      if (remove_password(client, input.value, input.len)) WRITE_OK(client);
      else {
        _write(client, "-This password cannot be found\r\n", 32);
      }
    } else if (streq(subcommand, "GENERATE")) {
      char value[33];
      generate_random_string(value, 32);

      char buf[41];
      sprintf(buf, "$32\r\n%s\r\n", value);

      _write(client, buf, 39);
    }
  } else {
    _write(client, "-Not allowed to use this command, need P_AUTH\r\n", 47);
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
  .subcommands = subcommands,
  .subcommand_count = 0,
  .run = run
};
