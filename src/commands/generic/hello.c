#include "../../../headers/telly.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

struct Value {
  uint32_t length;
  char *data;
};

static void run(struct CommandEntry entry) {
  if (!entry.client) return;
  const uint32_t arg_count = entry.data->arg_count;

  // arg_count != 0 && arg_count != 1 && arg_count != 3 && arg_count != 4 && arg_count != 6
  if (arg_count > 6 || arg_count == 2 || arg_count == 5) {
    WRONG_ARGUMENT_ERROR(entry.client, "HELLO", 5);
    return;
  }

  if (arg_count > 0) {
    const char *protover = entry.data->args[0].value;

    if (streq(protover, "2")) entry.client->protover = RESP2;
    else if (streq(protover, "3")) entry.client->protover = RESP3;
    else {
      _write(entry.client, "-Invalid protocol version\r\n", 27);
      return;
    }
  }

  char client_id[11];
  uint32_t client_id_len = sprintf(client_id, "%d", entry.client->id);

  char *protocol;
  uint32_t protocol_len;

  switch (entry.client->protover) {
    case RESP2:
      protocol = "RESP2";
      protocol_len = 5;
      break;

    case RESP3:
      protocol = "RESP3";
      protocol_len = 5;
      break;

    default:
      protocol = "";
      protocol_len = 0;
  }

  struct Value values[4][2] = {
    {{6, "server"}, {5, "telly"}},
    {{7, "version"}, {sizeof(VERSION) - 1, VERSION}},
    {{8, "protocol"}, {protocol_len, protocol}},
    {{9, "client id"}, {client_id_len, client_id}}
  };

  char buf[1024];

  switch (entry.client->protover) {
    case RESP2:
      memcpy(buf, "*8\r\n", 5);

      for (uint32_t i = 0; i < 4; ++i) {
        char element[128];
        sprintf(element, "$%d\r\n%s\r\n$%d\r\n%s\r\n", values[i][0].length, values[i][0].data, values[i][1].length, values[i][1].data);
        strcat(buf, element);
      }

      _write(entry.client, buf, strlen(buf));
      break;

    case RESP3:
      memcpy(buf, "%4\r\n", 5);

      for (uint32_t i = 0; i < 4; ++i) {
        char element[128];
        sprintf(element, "$%d\r\n%s\r\n$%d\r\n%s\r\n", values[i][0].length, values[i][0].data, values[i][1].length, values[i][1].data);
        strcat(buf, element);
      }

      _write(entry.client, buf, strlen(buf));
      break;
  }
}

const struct Command cmd_hello = {
  .name = "HELLO",
  .summary = "Handshakes with the tellydb server.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
