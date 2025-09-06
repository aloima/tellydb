#include <telly.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static inline uint8_t read_permissions_value(struct CommandEntry entry, const char *value) {
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
        const size_t nbytes = sprintf(entry.buffer, "-Invalid permission: '%c'\r\n", c);
        return -1;
      }
    }

    cval += 1;
  }

  return permissions;
}

static inline string_t add_pwd(struct CommandEntry entry) {
  if (entry.data->arg_count != 3) {
    PASS_NO_CLIENT(entry.client);
    return WRONG_ARGUMENT_ERROR("PWD ADD");
  }

  const uint8_t all = get_full_password()->permissions;
  const string_t data = entry.data->args[1];
  const char *value = entry.data->args[2].value;
  const uint8_t permissions = (streq(value, "all") ? all : read_permissions_value(entry, value));
  const uint8_t not_have = ~entry.password->permissions & permissions;

  if (not_have) {
    PASS_NO_CLIENT(entry.client);
    return RESP_ERROR_MESSAGE("Tried to give permissions your password do not have");
  }

  if (where_password(data.value, data.len) == -1) {
    add_password(entry.client, data, permissions);

    PASS_NO_CLIENT(entry.client);
    return RESP_OK();
  }

  return RESP_ERROR_MESSAGE("This password already exists");
}

static inline string_t edit_pwd(struct CommandEntry entry) {
  if (entry.data->arg_count != 3) {
    PASS_NO_CLIENT(entry.client);
    return WRONG_ARGUMENT_ERROR("PWD EDIT");
  }

  const string_t input = entry.data->args[1];
  const char *value = entry.data->args[2].value;

  struct Password *target = get_password(input.value, input.len);

  if (!target) {
    PASS_NO_CLIENT(entry.client);
    return RESP_ERROR_MESSAGE("This password does not exist");
  }

  const uint8_t permissions = read_permissions_value(entry, value);
  const uint8_t not_have = ~entry.password->permissions & permissions;

  if (not_have) {
    PASS_NO_CLIENT(entry.client);
    return RESP_ERROR_MESSAGE("Tried to give permissions your password do not have");
  }

  target->permissions = permissions;

  PASS_NO_CLIENT(entry.client);
  return RESP_OK();
}

static inline string_t remove_pwd(struct CommandEntry entry) {
  if (entry.data->arg_count != 2) {
    PASS_NO_CLIENT(entry.client);
    return WRONG_ARGUMENT_ERROR("PWD REMOVE");
  }

  const string_t input = entry.data->args[1];

  if (remove_password(entry.client, input.value, input.len) && entry.client) {
    PASS_NO_CLIENT(entry.client);
    return RESP_OK();
  }

  PASS_NO_CLIENT(entry.client);
  return RESP_ERROR_MESSAGE("This password cannot be found");
}

static inline string_t generate_pwd(struct CommandEntry entry) {
  PASS_NO_CLIENT(entry.client);

  char value[33];
  generate_random_string(value, 32);

  sprintf(entry.buffer, "$32\r\n%s\r\n", value);
  return CREATE_STRING(entry.buffer, 39);
}

static string_t run(struct CommandEntry entry) {
  if (entry.data->arg_count == 0) {
    PASS_NO_CLIENT(entry.client);
    return MISSING_SUBCOMMAND_ERROR("PWD");
  }

  const string_t subcommand_string = entry.data->args[0];
  char *subcommand = malloc(subcommand_string.len + 1);
  to_uppercase(subcommand_string.value, subcommand);

  string_t response;

  if (streq(subcommand, "ADD")) {
    response = add_pwd(entry);
  } else if (streq(subcommand, "EDIT")) {
    response = edit_pwd(entry);
  } else if (streq(subcommand, "REMOVE")) {
    response = remove_pwd(entry);
  } else if (streq(subcommand, "GENERATE")) {
    response = generate_pwd(entry);
  }

  free(subcommand);
  PASS_NO_CLIENT(entry.client);
  return INVALID_SUBCOMMAND_ERROR("PWD");
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
