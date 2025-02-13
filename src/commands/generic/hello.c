#include "../../../headers/telly.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

struct Value {
  uint32_t length;
  char *data;
};

static void run(struct Client *client, commanddata_t *command, __attribute__((unused)) struct Password *password) {
  if (client) {
    const uint32_t arg_count = command->arg_count;

    // arg_count != 0 && arg_count != 1 && arg_count != 3 && arg_count != 4 && arg_count != 6
    if (arg_count > 6 || arg_count == 2 || arg_count == 5) {
      WRONG_ARGUMENT_ERROR(client, "HELLO", 5);
      return;
    }

    if (command->arg_count > 0) {
      char *protover = command->args[0].value;

      if (streq(protover, "2")) client->protover = RESP2;
      else if (streq(protover, "3")) client->protover = RESP3;
      else {
        _write(client, "-Invalid protocol version\r\n", 27);
        return;
      }
    }

    char client_id[11];
    sprintf(client_id, "%d", client->id);

    char *protocol;

    switch (client->protover) {
      case RESP2:
        protocol = "RESP2";
        break;

      case RESP3:
        protocol = "RESP3";
        break;

      default:
        protocol = "";
    }

    struct Value values[4][2] = {
      {{6, "server"}, {5, "telly"}},
      {{7, "version"}, {strlen(VERSION), VERSION}},
      {{8, "protocol"}, {strlen(protocol), protocol}},
      {{9, "client id"}, {strlen(client_id), client_id}}
    };

    switch (client->protover) {
      case RESP2: {
        char buf[1024];
        memcpy(buf, "*8\r\n", 5);

        for (uint32_t i = 0; i < 4; ++i) {
          char element[128];
          sprintf(element, "$%d\r\n%s\r\n$%d\r\n%s\r\n", values[i][0].length, values[i][0].data, values[i][1].length, values[i][1].data);
          strcat(buf, element);
        }

        _write(client, buf, strlen(buf));
        break;
      }

      case RESP3: {
        char buf[1024];
        memcpy(buf, "%4\r\n", 5);

        for (uint32_t i = 0; i < 4; ++i) {
          char element[128];
          sprintf(element, "$%d\r\n%s\r\n$%d\r\n%s\r\n", values[i][0].length, values[i][0].data, values[i][1].length, values[i][1].data);
          strcat(buf, element);
        }

        _write(client, buf, strlen(buf));
        break;
      }
    }
  }
}

const struct Command cmd_hello = {
  .name = "HELLO",
  .summary = "Handshakes with the tellydb server.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
