#include "../../../headers/server.h"
#include "../../../headers/commands.h"
#include "../../../headers/utils.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count != 1) {
      const string_t subcommand_string = data->value.array[1]->value.string;
      char subcommand[subcommand_string.len + 1];
      to_uppercase(subcommand_string.value, subcommand);

      if (streq("ID", subcommand)) {
        char buf[14];
        const size_t nbytes = sprintf(buf, ":%d\r\n", client->id);
        _write(client, buf, nbytes);
      } else if (streq("INFO", subcommand)) {
        const char *lib_name = client->lib_name ? client->lib_name : "unspecified";
        const char *lib_ver = client->lib_ver ? client->lib_ver : "unspecified";

        char buf[512];
        const size_t buf_len = sprintf(buf, (
          "ID: %d\r\n"
          "Socket file descriptor: %d\r\n"
          "Connected at: %.24s\r\n"
          "Last used command: %s\r\n"
          "Library name: %s\r\n"
          "Library version: %s\r\n"
        ),
        client->id, client->connfd, ctime(&client->connected_at),
        client->command->name, lib_name, lib_ver);

        char res[1024];
        const size_t nbytes = sprintf(res, "$%ld\r\n%s\r\n", buf_len, buf);
        _write(client, res, nbytes);
      } else if (streq("SETINFO", subcommand)) {
        if (data->count == 4) {
          string_t property = data->value.array[2]->value.string;
          char property_value[property.len + 1];
          to_uppercase(property.value, property_value);

          if (streq(property_value, "LIB-NAME")) {
            string_t value = data->value.array[3]->value.string;
            const uint32_t value_size = value.len + 1;

            client->lib_name = client->lib_name ? realloc(client->lib_name, value_size) : malloc(value_size);
            memcpy(client->lib_name, value.value, value_size);

            _write(client, "+OK\r\n", 5);
          } else if (streq(property_value, "LIB-VERSION")) {
            string_t value = data->value.array[3]->value.string;
            const uint32_t value_size = value.len + 1;

            client->lib_ver = client->lib_ver ? realloc(client->lib_ver, value_size) : malloc(value_size);
            memcpy(client->lib_ver, value.value, value_size);

            _write(client, "+OK\r\n", 5);
          } else {
            _write(client, "-Unknown property\r\n", 19);
          }
        } else {
          WRONG_ARGUMENT_ERROR(client, "CLIENT SETINFO", 14);
        }
      }
    } else {
      WRONG_ARGUMENT_ERROR(client, "CLIENT", 6);
    }
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
    .name = "SETINFO",
    .summary = "Sets properties for the client.",
    .since = "0.1.2",
    .complexity = "O(1)"
  }
};

struct Command cmd_client = {
  .name = "CLIENT",
  .summary = "Gives information about client(s).",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = subcommands,
  .subcommand_count = 3,
  .run = run
};
