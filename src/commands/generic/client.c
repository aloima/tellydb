#include <telly.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

static inline string_t subcommand_id(struct Client *client, char *buffer) {
  const size_t nbytes = create_resp_integer(buffer, client->id);
  return CREATE_STRING(buffer, nbytes);
}

static inline string_t subcommand_info(struct Client *client, char *buffer) {
  const char *lib_name = client->lib_name ?: "unspecified";
  const char *lib_ver  = client->lib_ver ?: "unspecified";
  const char *protocol;

  static const char *protocols[] = {
    "unspecified",
    "unspecified",
    "RESP2",
    "RESP3"
  };

  if (client->protover < 4) {
    protocol = protocols[client->protover];
  } else {
    protocol = protocols[0];
  }

  char permissions[43];
  uint32_t permissions_len = 0;

  {
    const uint8_t value = client->password->permissions;

    if (value == 0) {
      memcpy(permissions, "None", 5);
    } else {
      const string_t perm_names[] = {
        {"read",   4}, {"write", 5}, {"client", 6},
        {"config", 6}, {"auth",  4}, {"server", 6}
      };

      const enum Permissions perm_masks[] = {
        P_READ, P_WRITE, P_CLIENT,
        P_CONFIG, P_AUTH, P_SERVER
      };

      for (uint32_t i = 0; i < 6; ++i) {
        if (value & perm_masks[i]) {
          memcpy(permissions + permissions_len, perm_names[i].value, perm_names[i].len);
          permissions_len += perm_names[i].len;

          memcpy(permissions + permissions_len, ", ", 2);
          permissions_len += 2;
        }
      }

      permissions[0] = (permissions[0] - 32);
      permissions_len -= 2;
    }
  }

  char connected_at[21];
  generate_date_string(connected_at, client->connected_at);

  char res[512];
  const size_t res_len = sprintf(res, (
    "ID: %" PRIu32 "\r\n"
    "Socket file descriptor: %" PRIi32 "\r\n"
    "Connected at: %.20s\r\n"
    "Last used command: %s\r\n"
    "Library name: %s\r\n"
    "Library version: %s\r\n"
    "Protocol: %s\r\n"
    "Permissions: %.*s\r\n"
  ), client->id, client->connfd, connected_at, client->command->name, lib_name, lib_ver, protocol, permissions_len, permissions);

  const size_t nbytes = create_resp_string(buffer, (string_t) {
    .value = res,
    .len = res_len
  });

  return CREATE_STRING(buffer, nbytes);
}

static string_t subcommand_lock(struct CommandEntry entry) {
  if (!(entry.password->permissions & P_CLIENT)) {
    PASS_NO_CLIENT(entry.client);
    return RESP_ERROR_MESSAGE("Not allowed to use this command, need P_CLIENT");
  }

  const int64_t id = strtoll(entry.data->args[1].value, NULL, 10);

  if ((id > UINT32_MAX) || (id < 0)) {
    PASS_NO_CLIENT(entry.client);
    return RESP_ERROR_MESSAGE("Specified ID is out of bounds for uint32_t");
  }

  struct Client *target = get_client_from_id(id);

  if (!target) {
    PASS_NO_CLIENT(entry.client);
    const size_t nbytes = sprintf(entry.buffer, "-There is no client whose ID is #%" PRIi64 "\r\n", id);
    return CREATE_STRING(entry.buffer, nbytes);
  } else if (target->password->permissions & P_CLIENT) {
    PASS_NO_CLIENT(entry.client);
    const size_t nbytes = sprintf(entry.buffer, "-Client #%" PRIi64 " has P_CLIENT, so cannot be locked\r\n", id);
    return CREATE_STRING(entry.buffer, nbytes);
  } else if (target->locked) {
    PASS_NO_CLIENT(entry.client);
    const size_t nbytes = sprintf(entry.buffer, "-Client #%" PRIi64 " is locked, so cannot be relocked\r\n", id);
    return CREATE_STRING(entry.buffer, nbytes);
  }

  target->locked = true;

  PASS_NO_CLIENT(entry.client);
  return RESP_OK();
}

static inline string_t subcommand_setinfo(struct CommandEntry entry) {
  if (entry.data->arg_count != 3) {
    return WRONG_ARGUMENT_ERROR("CLIENT SETINFO");
  }

  string_t property = entry.data->args[1];
  to_uppercase(property, property.value);

  if (streq(property.value, "LIB-NAME")) {
    string_t value = entry.data->args[2];
    const uint32_t value_size = value.len + 1;

    if (entry.client->lib_name) {
      free(entry.client->lib_name);
    }

    entry.client->lib_name = malloc(value_size);
    memcpy(entry.client->lib_name, value.value, value_size);

    return RESP_OK();
  } else if (streq(property.value, "LIB-VERSION")) {
    string_t value = entry.data->args[2];
    const uint32_t value_size = value.len + 1;

    if (entry.client->lib_ver) {
      free(entry.client->lib_ver);
    }

    entry.client->lib_ver = malloc(value_size);
    memcpy(entry.client->lib_ver, value.value, value_size);

    return RESP_OK();
  } else {
    return RESP_ERROR_MESSAGE("Unknown property");
  }
}

static inline string_t subcommand_kill(struct CommandEntry entry) {
  if (!(entry.password->permissions & P_CLIENT)) {
    PASS_NO_CLIENT(entry.client);
    return RESP_ERROR_MESSAGE("Not allowed to use this command, need P_CLIENT");
  }

  const int64_t id = strtoll(entry.data->args[1].value, NULL, 10);

  if ((id > UINT32_MAX) || (id < 0)) {
    PASS_NO_CLIENT(entry.client);
    return RESP_ERROR_MESSAGE("Specified ID is out of bounds for uint32_t");
  }

  struct Client *target = get_client_from_id(id);

  if (!target) {
    PASS_NO_CLIENT(entry.client);
    const size_t nbytes = sprintf(entry.buffer, "-There is no client whose ID is #%" PRIi64 "\r\n", id);
    return CREATE_STRING(entry.buffer, nbytes);;
  }

  if (target->password->permissions & P_CLIENT) {
    PASS_NO_CLIENT(entry.client);
    const size_t nbytes = sprintf(entry.buffer, "-Client #%" PRIi64 " has P_CLIENT, so cannot be killed\r\n", id);
    return CREATE_STRING(entry.buffer, nbytes);
  }

  terminate_connection(target->connfd);

  PASS_NO_CLIENT(entry.client);
  return RESP_OK();
}

static inline string_t subcommand_unlock(struct CommandEntry entry) {
  if (!(entry.password->permissions & P_CLIENT)) {
    PASS_NO_CLIENT(entry.client);
    return RESP_ERROR_MESSAGE("Not allowed to use this command, need P_CLIENT");
  }

  const int64_t id = strtoll(entry.data->args[1].value, NULL, 10);

  if ((id > UINT32_MAX) || (id < 0)) {
    PASS_NO_CLIENT(entry.client);
    return RESP_ERROR_MESSAGE("Specified ID is out of bounds for uint32_t");
  }

  struct Client *target = get_client_from_id(id);

  if (!target) {
    PASS_NO_CLIENT(entry.client);
    const size_t nbytes = sprintf(entry.buffer, "-There is no client whose ID is #%" PRIi64 "\r\n", id);
    return CREATE_STRING(entry.buffer, nbytes);
  } else if (!target->locked) {
    PASS_NO_CLIENT(entry.client);
    const size_t nbytes = sprintf(entry.buffer, "-Client #%" PRIi64 " is not locked, so cannot be unlocked\r\n", id);
    return CREATE_STRING(entry.buffer, nbytes);
  }

  target->locked = false;

  PASS_NO_CLIENT(entry.client);
  return RESP_OK();
}

static string_t run(struct CommandEntry entry) {
  if (entry.data->arg_count == 0) {
    PASS_NO_CLIENT(entry.client);
    return MISSING_SUBCOMMAND_ERROR("CLIENT");
  }

  const string_t subcommand = entry.data->args[0];
  to_uppercase(subcommand, subcommand.value);

  string_t response;

  if (streq("ID", subcommand.value) && entry.client) {
    response = subcommand_id(entry.client, entry.buffer);
  } else if (streq("INFO", subcommand.value) && entry.client) {
    response = subcommand_info(entry.client, entry.buffer);
  } else if (streq("LOCK", subcommand.value)) {
    response = subcommand_lock(entry);
  } else if (streq("SETINFO", subcommand.value) && entry.client) {
    response = subcommand_setinfo(entry);
  } else if (streq("KILL", subcommand.value)) {
    response = subcommand_kill(entry);
  } else if (streq("UNLOCK", subcommand.value)) {
    response = subcommand_unlock(entry);
  } else if (entry.client) {
    response = INVALID_SUBCOMMAND_ERROR("CLIENT");
  }

  return response;
}

static struct Subcommand subcommands[] = {
  (struct Subcommand) {
    .name = "ID",
    .summary = "Returns ID number of client.",
    .since = "0.1.0",
    .complexity = "O(1)"
  },
  (struct Subcommand) {
    .name = "INFO",
    .summary = "Returns information about the client.",
    .since = "0.1.0",
    .complexity = "O(1)"
  },
  (struct Subcommand) {
    .name = "LOCK",
    .summary = "Locks specified client.",
    .since = "0.1.8",
    .complexity = "O(1)"
  },
  (struct Subcommand) {
    .name = "SETINFO",
    .summary = "Sets properties for the client.",
    .since = "0.1.2",
    .complexity = "O(1)"
  },
  (struct Subcommand) {
    .name = "KILL",
    .summary = "Kills specified client.",
    .since = "0.1.8",
    .complexity = "O(1)"
  },
  (struct Subcommand) {
    .name = "UNLOCK",
    .summary = "Unlocks specified client.",
    .since = "0.1.8",
    .complexity = "O(1)"
  },
};

const struct Command cmd_client = {
  .name = "CLIENT",
  .summary = "Main command of client(s).",
  .since = "0.1.0",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .subcommands = subcommands,
  .subcommand_count = 6,
  .run = run
};
