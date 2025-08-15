#include <telly.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

static inline void subcommand_id(struct Client *client) {
  char buf[14];
  const size_t nbytes = create_resp_integer(buf, client->id);
  _write(client, buf, nbytes);
}

static inline void subcommand_info(struct Client *client) {
  const char *lib_name = client->lib_name ? client->lib_name : "unspecified";
  const char *lib_ver = client->lib_ver ? client->lib_ver : "unspecified";
  char *protocol;

  switch (client->protover) {
    case RESP2:
      protocol = "RESP2";
      break;

    case RESP3:
      protocol = "RESP3";
      break;

    default:
      protocol = "unspecified";
  }

  char permissions[44];
  permissions[0] = '\0';

  {
    const uint8_t value = client->password->permissions;
    uint32_t length = 0;

    if (value == 0) {
      memcpy(permissions, "None", 5);
    } else {
      if (value & P_READ) strcat(permissions, "read, "), length += 6;
      if (value & P_WRITE) strcat(permissions, "write, "), length += 7;
      if (value & P_CLIENT) strcat(permissions, "client, "), length += 8;
      if (value & P_CONFIG) strcat(permissions, "config, "), length += 8;
      if (value & P_AUTH) strcat(permissions, "auth, "), length += 6;
      if (value & P_SERVER) strcat(permissions, "server, "), length += 8;

      permissions[0] = toupper(permissions[0]);
      permissions[length - 2] = '\0';
    }
  }

  char connected_at[21];
  generate_date_string(connected_at, client->connected_at);

  char buf[512];
  const size_t buf_len = sprintf(buf, (
    "ID: %u\r\n"
    "Socket file descriptor: %d\r\n"
    "Connected at: %.20s\r\n"
    "Last used command: %s\r\n"
    "Library name: %s\r\n"
    "Library version: %s\r\n"
    "Protocol: %s\r\n"
    "Permissions: %s\r\n"
  ), client->id, client->connfd, connected_at, client->command->name, lib_name, lib_ver, protocol, permissions);

  char res[1024];
  const size_t nbytes = create_resp_string(res, (string_t) {
    .value = buf,
    .len = buf_len
  });

  _write(client, res, nbytes);
}

static void subcommand_lock(struct CommandEntry entry) {
  if (entry.password->permissions & P_CLIENT) {
    if (!entry.client) return;

    WRITE_ERROR_MESSAGE(entry.client, "Not allowed to use this command, need P_CLIENT");
    return;
  }

  const long id = atol(entry.data->args[1].value);

  if ((id > UINT32_MAX) || (id < 0)) {
    if (!entry.client) return;

    WRITE_ERROR_MESSAGE(entry.client, "Specified ID is out of bounds for uint32_t");
    return;
  }

  struct Client *target = get_client_from_id(id);

  if (!target) {
    if (!entry.client) return;

    char buf[46];
    const size_t nbytes = sprintf(buf, "-There is no client whose ID is #%ld\r\n", id);
    _write(entry.client, buf, nbytes);
    return;
  }

  if (target->password->permissions & P_CLIENT) {
    if (!entry.client) return;

    char buf[56];
    const size_t nbytes = sprintf(buf, "-Client #%ld has P_CLIENT, so cannot be locked\r\n", id);
    _write(entry.client, buf, nbytes);
    return;
  } else if (target->locked) {
    if (!entry.client) return;

    char buf[55];
    const size_t nbytes = sprintf(buf, "-Client #%ld is locked, so cannot be relocked\r\n", id);
    _write(entry.client, buf, nbytes);
    return;
  }

  target->locked = true;

  if (entry.client) {
    WRITE_OK(entry.client);
  }
}

static inline void subcommand_setinfo(struct CommandEntry entry) {
  if (entry.data->arg_count != 3) {
    WRONG_ARGUMENT_ERROR(entry.client, "CLIENT SETINFO");
    return;
  }

  string_t property = entry.data->args[1];
  char property_value[property.len + 1];
  to_uppercase(property.value, property_value);

  if (streq(property_value, "LIB-NAME")) {
    string_t value = entry.data->args[2];
    const uint32_t value_size = value.len + 1;

    if (entry.client->lib_name) free(entry.client->lib_name);
    entry.client->lib_name = malloc(value_size);
    memcpy(entry.client->lib_name, value.value, value_size);

    WRITE_OK(entry.client);
  } else if (streq(property_value, "LIB-VERSION")) {
    string_t value = entry.data->args[2];
    const uint32_t value_size = value.len + 1;

    if (entry.client->lib_ver) free(entry.client->lib_ver);
    entry.client->lib_ver = malloc(value_size);
    memcpy(entry.client->lib_ver, value.value, value_size);

    WRITE_OK(entry.client);
  } else {
    WRITE_ERROR_MESSAGE(entry.client, "Unknown property");
  }
}

static inline void subcommand_kill(struct CommandEntry entry) {
  if (!(entry.password->permissions & P_CLIENT)) {
    if (!entry.client) return;

    WRITE_ERROR_MESSAGE(entry.client, "Not allowed to use this command, need P_CLIENT");
    return;
  }

  const long id = atol(entry.data->args[1].value);

  if ((id > UINT32_MAX) || (id < 0)) {
    if (!entry.client) return;

    WRITE_ERROR_MESSAGE(entry.client, "Specified ID is out of bounds for uint32_t");
    return;
  }

  struct Client *target = get_client_from_id(id);

  if (!target) {
    if (!entry.client) return;

    char buf[46];
    const size_t nbytes = sprintf(buf, "-There is no client whose ID is #%ld\r\n", id);
    _write(entry.client, buf, nbytes);
    return;
  }

  if (target->password->permissions & P_CLIENT) {
    if (!entry.client) return;

    char buf[56];
    const size_t nbytes = sprintf(buf, "-Client #%ld has P_CLIENT, so cannot be killed\r\n", id);
    _write(entry.client, buf, nbytes);
    return;
  }

  terminate_connection(target->connfd);

  if (entry.client) {
    WRITE_OK(entry.client);
  }
}

static inline void subcommand_unlock(struct CommandEntry entry) {
  if (!(entry.password->permissions & P_CLIENT)) {
    if (!entry.client) return;

    WRITE_ERROR_MESSAGE(entry.client, "Not allowed to use this command, need P_CLIENT");
    return;
  }

  const long id = atol(entry.data->args[1].value);

  if ((id > UINT32_MAX) || (id < 0)) {
    if (!entry.client) return;

    WRITE_ERROR_MESSAGE(entry.client, "Specified ID is out of bounds for uint32_t");
    return;
  }

  struct Client *target = get_client_from_id(id);

  if (!target) {
    if (!entry.client) return;

    char buf[46];
    const size_t nbytes = sprintf(buf, "-There is no client whose ID is #%ld\r\n", id);
    _write(entry.client, buf, nbytes);

    return;
  }

  if (!target->locked) {
    if (!entry.client) return;

    char buf[59];
    const size_t nbytes = sprintf(buf, "-Client #%ld is not locked, so cannot be unlocked\r\n", id);
    _write(entry.client, buf, nbytes);
    return;
  }

  target->locked = false;

  if (entry.client) {
    WRITE_OK(entry.client);
  }
}

static void run(struct CommandEntry entry) {
  if (entry.data->arg_count == 0) {
    if (entry.client) {
      MISSING_SUBCOMMAND_ERROR(entry.client, "CLIENT");
    }

    return;
  }

  const string_t subcommand_string = entry.data->args[0];
  char *subcommand = malloc(subcommand_string.len + 1);
  to_uppercase(subcommand_string.value, subcommand);

  if (streq("ID", subcommand) && entry.client) {
    subcommand_id(entry.client);
  } else if (streq("INFO", subcommand) && entry.client) {
    subcommand_info(entry.client);
  } else if (streq("LOCK", subcommand)) {
    subcommand_lock(entry);
  } else if (streq("SETINFO", subcommand) && entry.client) {
    subcommand_setinfo(entry);
  } else if (streq("KILL", subcommand)) {
    subcommand_kill(entry);
  } else if (streq("UNLOCK", subcommand)) {
    subcommand_unlock(entry);
  } else if (entry.client) {
    INVALID_SUBCOMMAND_ERROR(entry.client, "CLIENT");
  }

  free(subcommand);
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
