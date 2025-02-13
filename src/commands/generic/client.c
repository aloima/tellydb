#include "../../../headers/telly.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

static void run(struct Client *client, commanddata_t *command, __attribute__((unused)) struct Password *password) {
  if (command->arg_count != 0) {
    const string_t subcommand_string = command->args[0];
    char subcommand[subcommand_string.len + 1];
    to_uppercase(subcommand_string.value, subcommand);

    if (streq("ID", subcommand) && client) {
      char buf[14];
      const size_t nbytes = sprintf(buf, ":%d\r\n", client->id);
      _write(client, buf, nbytes);
    } else if (streq("INFO", subcommand) && client) {
      const char *lib_name = client->lib_name ? client->lib_name : "unspecified";
      const char *lib_ver = client->lib_ver ? client->lib_ver : "unspecified";
      char *protocol = "unspecified";

      switch (client->protover) {
        case RESP2:
          protocol = "RESP2";
          break;

        case RESP3:
          protocol = "RESP3";
          break;
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

      char buf[512];

      char connected_at[21];
      generate_date_string(connected_at, client->connected_at);

      const size_t buf_len = sprintf(buf, (
        "ID: %d\r\n"
        "Socket file descriptor: %d\r\n"
        "Connected at: %.20s\r\n"
        "Last used command: %s\r\n"
        "Library name: %s\r\n"
        "Library version: %s\r\n"
        "Protocol: %s\r\n"
        "Permissions: %s\r\n"
      ), client->id, client->connfd, connected_at, client->command->name, lib_name, lib_ver, protocol, permissions);

      char res[1024];
      const size_t nbytes = sprintf(res, "$%ld\r\n%s\r\n", buf_len, buf);
      _write(client, res, nbytes);
    }  else if (streq("LOCK", subcommand)) {
      if (password->permissions & P_CLIENT) {
        const long id = atol(command->args[1].value);

        if ((id > UINT32_MAX) || (id < 0)) {
          if (client) _write(client, "-Specified ID is out of bounds for uint32_t\r\n", 45);
        } else {
          struct Client *target = get_client_from_id(id);

          if (target) {
            if (target->password->permissions & P_CLIENT) {
              if (client) {
                char buf[56];
                const size_t nbytes = sprintf(buf, "-Client #%ld has P_CLIENT, so cannot be locked\r\n", id);
                _write(client, buf, nbytes);
              }
            } else if (target->locked) {
              if (client) {
                char buf[55];
                const size_t nbytes = sprintf(buf, "-Client #%ld is locked, so cannot be relocked\r\n", id);
                _write(client, buf, nbytes);
              }
            } else {
              target->locked = true;
              if (client) WRITE_OK(client);
            }
          } else if (client) {
            char buf[46];
            const size_t nbytes = sprintf(buf, "-There is no client whose ID is #%ld\r\n", id);
            _write(client, buf, nbytes);
          }
        }
      } else if (client) {
        _write(client, "-Not allowed to use this command, need P_CLIENT\r\n", 49);
      }
    } else if (streq("SETINFO", subcommand) && client) {
      if (command->arg_count == 3) {
        string_t property = command->args[1];
        char property_value[property.len + 1];
        to_uppercase(property.value, property_value);

        if (streq(property_value, "LIB-NAME")) {
          string_t value = command->args[2];
          const uint32_t value_size = value.len + 1;

          if (client->lib_name) free(client->lib_name);
          client->lib_name = malloc(value_size);
          memcpy(client->lib_name, value.value, value_size);

          WRITE_OK(client);
        } else if (streq(property_value, "LIB-VERSION")) {
          string_t value = command->args[2];
          const uint32_t value_size = value.len + 1;

          if (client->lib_ver) free(client->lib_ver);
          client->lib_ver = malloc(value_size);
          memcpy(client->lib_ver, value.value, value_size);

          WRITE_OK(client);
        } else {
          _write(client, "-Unknown property\r\n", 19);
        }
      } else {
        WRONG_ARGUMENT_ERROR(client, "CLIENT SETINFO", 14);
      }
    } else if (streq("KILL", subcommand)) {
      if (password->permissions & P_CLIENT) {
        const long id = atol(command->args[1].value);

        if ((id > UINT32_MAX) || (id < 0)) {
          if (client) _write(client, "-Specified ID is out of bounds for uint32_t\r\n", 45);
        } else {
          struct Client *target = get_client_from_id(id);

          if (target) {
            if (target->password->permissions & P_CLIENT) {
              if (client) {
                char buf[56];
                const size_t nbytes = sprintf(buf, "-Client #%ld has P_CLIENT, so cannot be killed\r\n", id);
                _write(client, buf, nbytes);
              }
            } else {
              terminate_connection(target->connfd);
              if (client) WRITE_OK(client);
            }
          } else if (client) {
            char buf[46];
            const size_t nbytes = sprintf(buf, "-There is no client whose ID is #%ld\r\n", id);
            _write(client, buf, nbytes);
          }
        }
      } else if (client) {
        _write(client, "-Not allowed to use this command, need P_CLIENT\r\n", 49);
      }
    } else if (streq("UNLOCK", subcommand)) {
      if (password->permissions & P_CLIENT) {
        const long id = atol(command->args[1].value);

        if ((id > UINT32_MAX) || (id < 0)) {
          if (client) _write(client, "-Specified ID is out of bounds for uint32_t\r\n", 45);
        } else {
          struct Client *target = get_client_from_id(id);

          if (target) {
            if (!target->locked) {
              if (client) {
                char buf[59];
                const size_t nbytes = sprintf(buf, "-Client #%ld is not locked, so cannot be unlocked\r\n", id);
                _write(client, buf, nbytes);
              }
            } else {
              target->locked = false;
              if (client) WRITE_OK(client);
            }
          } else if (client) {
            char buf[46];
            const size_t nbytes = sprintf(buf, "-There is no client whose ID is #%ld\r\n", id);
            _write(client, buf, nbytes);
          }
        }
      } else if (client) {
        _write(client, "-Not allowed to use this command, need P_CLIENT\r\n", 49);
      }
    }
  } else if (client) {
    WRONG_ARGUMENT_ERROR(client, "CLIENT", 6);
  }
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
  .subcommands = subcommands,
  .subcommand_count = 6,
  .run = run
};
