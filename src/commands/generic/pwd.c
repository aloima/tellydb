#include "../../../headers/server.h"
#include "../../../headers/commands.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

static void generate_random(char *dest, size_t length) {
  const char charset[] = "0123456789"
                         "abcdefghijklmnopqrstuvwxyz"
                         "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  while (length-- > 0) {
    const size_t index = (double) rand() / RAND_MAX * (sizeof(charset) - 1);
    *dest++ = charset[index];
  }

  *dest = '\0';
}

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (command->arg_count == 0) {
    WRONG_ARGUMENT_ERROR(client, "AUTH", 4);
    return;
  }

  if (password->permissions & P_AUTH) {
    const string_t subcommand_string = command->args[0];
    char subcommand[subcommand_string.len + 1];
    to_uppercase(subcommand_string.value, subcommand);

    if (streq(subcommand, "ADD")) {
      if (command->arg_count != 3) {
        WRONG_ARGUMENT_ERROR(client, "AUTH ADD", 8);
        return;
      }

      const string_t data = command->args[1];
      char *permissions_value = command->args[2].value;
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
            return;
          }
        }

        permissions_value += 1;
      }

      const uint8_t not_has = ~password->permissions & permissions;

      if (not_has) {
        _write(client, "-Tried to give permissions your password do not have\r\n", 54);
        return;
      }

      if (where_password(data.value) == -1) {
        add_password(client, data, permissions);
        WRITE_OK(client);
      } else {
        _write(client, "-This password is already exist\r\n", 33);
      }
    } else if (streq(subcommand, "REMOVE")) {
      if (command->arg_count != 2) {
        WRONG_ARGUMENT_ERROR(client, "AUTH REMOVE", 11);
        return;
      }

      const char *value = command->args[1].value;
      if (remove_password(client, value)) WRITE_OK(client);
      else {
        _write(client, "-This password cannot be found\r\n", 32);
      }
    } else if (streq(subcommand, "GENERATE")) {
      char value[33];
      generate_random(value, 32);

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

struct Command cmd_pwd = {
  .name = "PWD",
  .summary = "Allows to manage passwords.",
  .since = "0.1.7",
  .complexity = "O(1)",
  .subcommands = subcommands,
  .subcommand_count = 0,
  .run = run
};
