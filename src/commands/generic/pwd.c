#include <telly.h>

static constexpr const enum Permissions permissions_mapping[] = {
  ['r'] = P_READ,
  ['w'] = P_WRITE,
  ['c'] = P_CLIENT,
  ['o'] = P_CONFIG,
  ['a'] = P_AUTH,
  ['s'] = P_SERVER,
};

typedef struct {
  int status;

  union {
    uint8_t permissions;
    string_t error;
  } response;
} PermissionValue;

static inline PermissionValue read_permissions_value(struct CommandEntry *entry, const char *value) {
  int permissions = 0;
  char *cval = (char *) value;
  char c;

  while ((c = *cval) != '\0') {
    const enum Permissions data = permissions_mapping[c];

    if (data == 0) {
      permissions |= data;
    } else {
      const size_t nbytes = sprintf(entry->client->write_buf, "-Invalid permission: '%c'\r\n", c);

      return (PermissionValue) {-1, {
        .error = CREATE_STRING(entry->client->write_buf, nbytes)
      }};
    }

    cval += 1;
  }

  return (PermissionValue) {0, {
    .permissions = permissions
  }};
}

static inline string_t add_pwd(struct CommandEntry *entry) {
  if (entry->args->count != 3) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("PWD ADD");
  }

  const string_t data = entry->args->data[1];

  const char *value = entry->args->data[2].value;
  int permissions = -1;

  if (streq(value, "all")) {
    const uint8_t all_permissions = get_full_password()->permissions;
    permissions = all_permissions;
  } else {
    const PermissionValue permission_value = read_permissions_value(entry, value);

    if (permission_value.status == -1) {
      PASS_NO_CLIENT(entry->client);
      return permission_value.response.error;
    }

    permissions = permission_value.response.permissions;
  }

  const uint8_t not_have = ~entry->password->permissions & permissions;

  if (not_have) {
    PASS_NO_CLIENT(entry->client);
    return RESP_ERROR_MESSAGE("Tried to give permissions your password do not have");
  }

  if (where_password(data.value, data.len) == -1) {
    const int added = add_password(entry->client, data, permissions);

    PASS_NO_CLIENT(entry->client);
    return (added == 0) ? RESP_OK() : RESP_ERROR_MESSAGE("Password cannot be created");
  }

  return RESP_ERROR_MESSAGE("This password already exists");
}

static inline string_t edit_pwd(struct CommandEntry *entry) {
  if (entry->args->count != 3) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("PWD EDIT");
  }

  const string_t input = entry->args->data[1];
  const int target = where_password(input.value, input.len);

  if (target == -1) {
    PASS_NO_CLIENT(entry->client);
    return RESP_ERROR_MESSAGE("This password does not exist");
  }

  const char *value = entry->args->data[2].value;
  int permissions = -1;

  if (streq(value, "all")) {
    const uint8_t all_permissions = get_full_password()->permissions;
    permissions = all_permissions;
  } else {
    const PermissionValue permission_value = read_permissions_value(entry, value);

    if (permission_value.status == -1) {
      PASS_NO_CLIENT(entry->client);
      return permission_value.response.error;
    }

    permissions = permission_value.response.permissions;
  }

  const uint8_t not_have = ~entry->password->permissions & permissions;

  if (not_have) {
    PASS_NO_CLIENT(entry->client);
    return RESP_ERROR_MESSAGE("Tried to give permissions your password do not have");
  }

  struct Password **passwords = get_passwords();
  passwords[target]->permissions = permissions;

  PASS_NO_CLIENT(entry->client);
  return RESP_OK();
}

static inline string_t remove_pwd(struct CommandEntry *entry) {
  if (entry->args->count != 2) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("PWD REMOVE");
  }

  const string_t input = entry->args->data[1];

  if (remove_password(entry->client, input.value, input.len) == -1) {
    PASS_NO_CLIENT(entry->client);
    return RESP_ERROR_MESSAGE("This password cannot be found");
  }

  PASS_NO_CLIENT(entry->client);
  return RESP_OK();
}

static inline string_t generate_pwd(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  char value[33];
  generate_random_string(value, 32);

  ASSERT(sprintf(entry->client->write_buf, "$32\r\n%s\r\n", value), ==, 39);
  return CREATE_STRING(entry->client->write_buf, 39);
}

static string_t run(struct CommandEntry *entry) {
  if (entry->args->count == 0) {
    PASS_NO_CLIENT(entry->client);
    return MISSING_SUBCOMMAND_ERROR("PWD");
  }

  const string_t subcommand = entry->args->data[0];
  to_uppercase(subcommand, subcommand.value);

  string_t response;

  if (streq(subcommand.value, "ADD")) {
    response = add_pwd(entry);
  } else if (streq(subcommand.value, "EDIT")) {
    response = edit_pwd(entry);
  } else if (streq(subcommand.value, "REMOVE")) {
    response = remove_pwd(entry);
  } else if (streq(subcommand.value, "GENERATE")) {
    response = generate_pwd(entry);
  } else {
    PASS_NO_CLIENT(entry->client);
    return INVALID_SUBCOMMAND_ERROR("PWD");
  }

  PASS_NO_CLIENT(entry->client);
  return response;
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
  .flags.value = CMD_FLAG_NO_FLAG,
  .subcommands = subcommands,
  .subcommand_count = 0,
  .run = run
};
