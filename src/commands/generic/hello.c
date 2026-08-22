#include <telly.h>

static const string_t PROTOCOLS[] = {
  CREATE_SIZED_STRING("unspecified"),
  CREATE_SIZED_STRING("unspecified"),
  CREATE_SIZED_STRING("RESP2"),
  CREATE_SIZED_STRING("RESP3")
};

static inline bool determine_protover(enum ProtocolVersion *protover, const string_t value) {
  if (SSTREQ(value, CREATE_SIZED_STRING("2"))) {
    *protover = RESP2;
    return true;
  } else if (SSTREQ(value, CREATE_SIZED_STRING("3"))) {
    *protover = RESP3;
    return true;
  }

  return false;
}

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 1) {
    return WRONG_ARGUMENT_ERROR("HELLO");
  }

  if (!determine_protover(&entry->client->protover, entry->args->data[0])) {
    return RESP_ERROR_MESSAGE("Invalid protocol version");
  }

  char client_id[11];
  const uint32_t client_id_len = ltoa(entry->client->id, client_id);
  ASSERT(client_id_len, <=, 11U);

  const enum ProtocolVersion protover = entry->client->protover;

  string_t values[4][2] = {
    {CREATE_SIZED_STRING("server"),    CREATE_SIZED_STRING("telly")},
    {CREATE_SIZED_STRING("version"),   CREATE_SIZED_STRING(VERSION)},
    {CREATE_SIZED_STRING("protocol"),  (protover < 4) ? PROTOCOLS[protover] : PROTOCOLS[0]},
    {CREATE_SIZED_STRING("client id"), CREATE_STRING(client_id, client_id_len)}
  };

  char *buf = entry->client->write_buf;
  uint32_t at;

  switch (protover) {
    case RESP2:
      memcpy(buf, "*8\r\n", 4);
      at = 4;
      break;

    case RESP3:
      memcpy(buf, "%4\r\n", 4);
      at = 4;
      break;
  }

  for (size_t i = 0; i < 4; ++i) {
    at += create_resp_string(buf + at, values[i][0]); // key
    at += create_resp_string(buf + at, values[i][1]); // value
  }

  return CREATE_STRING(buf, at);
}

const struct Command cmd_hello = {
  .name = "HELLO",
  .summary = "Handshakes with the tellydb server.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .flags.value = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = NULL
};
