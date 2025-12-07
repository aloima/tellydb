#include <telly.h>

#include <string.h>
#include <stdint.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  const uint32_t arg_count = entry->data->arg_count;

  if (arg_count != 1) {
    return WRONG_ARGUMENT_ERROR("HELLO");
  }

  const char *protover = entry->data->args[0].value;

  if (streq(protover, "2")) {
    entry->client->protover = RESP2;
  } else if (streq(protover, "3")) {
    entry->client->protover = RESP3;
  } else {
    return RESP_ERROR_MESSAGE("Invalid protocol version");
  }

  char client_id[11];
  const uint32_t client_id_len = ltoa(entry->client->id, client_id);

  string_t values[4][2] = {
    {{"server",    6}, {"telly",   5}},
    {{"version",   7}, {VERSION,   sizeof(VERSION) - 1}},
    {{"protocol",  8}},
    {{"client id", 9}, {client_id, client_id_len}}
  };

  static const string_t protocols[] = {
    {"unspecified", 11},
    {"unspecified", 11},
    {"RESP2", 5},
    {"RESP3", 5}
  };

  if (entry->client->protover < 4) {
    values[2][1] = protocols[entry->client->protover];
  } else {
    values[2][1] = protocols[0];
  }

  char *buf = entry->client->write_buf;
  uint32_t at;

  switch (entry->client->protover) {
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

  return CREATE_STRING(buf, at);
}

const struct Command cmd_hello = {
  .name = "HELLO",
  .summary = "Handshakes with the tellydb server.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .flags = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
