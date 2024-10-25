#include "../../../headers/server.h"
#include "../../../headers/commands.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count > 7 || data->count == 6) {
      WRONG_ARGUMENT_ERROR(client, "HELLO", 5);
      return;
    }

    if (data->count > 1) {
      char *protover = data->value.array[1]->value.string.value;

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
    }

    char *values[4][2] = {
      {"server", "telly"},
      {"version", VERSION},
      {"protocol", protocol},
      {"client id", client_id}
    };

    switch (client->protover) {
      case RESP2: {
        char buf[1024];
        memcpy(buf, "*8\r\n", 5);

        for (uint32_t i = 0; i < 4; ++i) {
          char element[128];
          sprintf(element, "+%s\r\n+%s\r\n", values[i][0], values[i][1]);
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
          sprintf(element, "+%s\r\n+%s\r\n", values[i][0], values[i][1]);
          strcat(buf, element);
        }

        _write(client, buf, strlen(buf));
        break;
      }
    }
  }
}

struct Command cmd_hello = {
  .name = "HELLO",
  .summary = "Handshakes with the tellydb server.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
