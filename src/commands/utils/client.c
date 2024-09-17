#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, __attribute__((unused)) struct Configuration *conf) {
  if (client != NULL) {
    if (data->count != 1) {
      string_t subcommand_string = data->value.array[1]->value.string;
      char subcommand[subcommand_string.len + 1];
      to_uppercase(subcommand_string.value, subcommand);

      if (streq("ID", subcommand)) {
        const uint32_t len = 3 + get_digit_count(client->id);
        char res[len + 1];
        sprintf(res, ":%d\r\n", client->id);

        write(client->connfd, res, len);
      } else if (streq("INFO", subcommand)) {
        const char *lib_name = client->lib_name ? client->lib_name : "unspecified";
        const char *lib_ver = client->lib_ver ? client->lib_ver : "unspecified";

        char buf[512];
        sprintf(buf, (
          "id: %d\r\n"
          "socket file descriptor: %d\r\n"
          "connected at: %.24s\r\n"
          "last used command: %s\r\n"
          "library name: %s\r\n"
          "library version: %s\r\n"
        ),
        client->id, client->connfd, ctime(&client->connected_at),
        client->command->name, lib_name, lib_ver);

        char res[1024];
        sprintf(res, "$%ld\r\n%s\r\n", strlen(buf), buf);

        write(client->connfd, res, strlen(res));
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

            write(client->connfd, "+OK\r\n", 5);
          } else if (streq(property_value, "LIB-VERSION")) {
            string_t value = data->value.array[3]->value.string;
            const uint32_t value_size = value.len + 1;

            client->lib_ver = client->lib_ver ? realloc(client->lib_ver, value_size) : malloc(value_size);
            memcpy(client->lib_ver, value.value, value_size);

            write(client->connfd, "+OK\r\n", 5);
          } else {
            write(client->connfd, "-Unknown property\r\n", 19);
          }
        } else {
          write(client->connfd, "-Wrong argument count for 'CLIENT SETINFO' command\r\n", 52);
        }
      }
    } else {
      write(client->connfd, "-Wrong argument count for 'CLIENT' command\r\n", 44);
    }
  }
}

static struct Subcommand subcommands[] = {
  (struct Subcommand) {
    .name = "ID",
    .summary = "Returns ID number of client.",
    .since = "1.0.0",
    .complexity = "O(1)"
  },
  (struct Subcommand) {
    .name = "INFO",
    .summary = "Returns information about the client.",
    .since = "1.0.0",
    .complexity = "O(1)"
  },
  (struct Subcommand) {
    .name = "SETINFO",
    .summary = "Sets properties for the client.",
    .since = "1.0.2",
    .complexity = "O(1)"
  }
};

struct Command cmd_client = {
  .name = "CLIENT",
  .summary = "Gives information about client(s).",
  .since = "1.0.0",
  .complexity = "O(1)",
  .subcommands = subcommands,
  .subcommand_count = 3,
  .run = run
};
