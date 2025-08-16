#include <telly.h>

#include <string.h>
#include <stdint.h>

static void run(struct CommandEntry entry) {
  if (!entry.client) return;
  const uint32_t arg_count = entry.data->arg_count;

  if (arg_count != 1) {
    WRONG_ARGUMENT_ERROR(entry.client, "HELLO");
    return;
  }

  const char *protover = entry.data->args[0].value;

  if (streq(protover, "2")) {
    entry.client->protover = RESP2;
  } else if (streq(protover, "3")) {
    entry.client->protover = RESP3;
  } else {
    WRITE_ERROR_MESSAGE(entry.client, "Invalid protocol version");
    return;
  }

  char client_id[11];
  const uint32_t client_id_len = ltoa(entry.client->id, client_id);

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

  string_t values[4][2] = {
    {{"server", 6}, {"telly", 5}},
    {{"version", 7}, {VERSION, sizeof(VERSION) - 1}},
    {{"protocol", 8}, {protocol, protocol_len}},
    {{"client id", 9}, {client_id, client_id_len}}
  };

  char buf[1024];
  uint32_t at;

  switch (entry.client->protover) {
    case RESP2:
      memcpy(buf, "*8\r\n", 4);
      at = 4;
      break;

    case RESP3:
      memcpy(buf, "%4\r\n", 4);
      at = 4;
      break;
  }

  at += create_resp_string(buf + at, values[0][0]);
  at += create_resp_string(buf + at, values[0][1]);
  at += create_resp_string(buf + at, values[1][0]);
  at += create_resp_string(buf + at, values[1][1]);
  at += create_resp_string(buf + at, values[2][0]);
  at += create_resp_string(buf + at, values[2][1]);
  at += create_resp_string(buf + at, values[3][0]);
  at += create_resp_string(buf + at, values[3][1]);

  _write(entry.client, buf, at);
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
