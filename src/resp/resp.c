#include <telly.h>

#include <stdbool.h>
#include <stdint.h>

extern bool check_crlf(Client *client, char *buf, int32_t *at, int32_t *size);

static inline bool get_resp_command_name(Arena *arena, Client *client, string_t *name, char *buf, int32_t *at, int32_t *size) {
  char *c;
  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(*c != RDT_BSTRING)) THROW_RESP_ERROR(client->id);

  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(!('0' <= *c && *c <= '9'))) THROW_RESP_ERROR(client->id);

  do {
    name->len = (name->len * 10) + (*c - '0');
    TAKE_BYTES(c, 1, false);
  } while ('0' <= *c && *c <= '9');

  if (VERY_UNLIKELY(*c != '\r')) THROW_RESP_ERROR(client->id);

  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(*c != '\n')) THROW_RESP_ERROR(client->id);

  name->value = arena_alloc(arena, name->len * sizeof(char));

  char *name_raw;
  TAKE_BYTES(name_raw, name->len, false);
  memcpy(name->value, name_raw, name->len);
  name->value[name->len] = '\0';

  if (!check_crlf(client, buf, at, size)) THROW_RESP_ERROR(client->id);
  return true;
}

static inline bool get_resp_command_argument(Arena *arena, Client *client, string_t *arg, char *buf, int32_t *at, int32_t *size) {
  char *c;
  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(*c != RDT_BSTRING)) THROW_RESP_ERROR(client->id);

  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(!('0' <= *c && *c <= '9'))) THROW_RESP_ERROR(client->id);

  do {
    arg->len = (arg->len * 10) + (*c - '0');
    TAKE_BYTES(c, 1, false);
  } while ('0' <= *c && *c <= '9');

  if (VERY_UNLIKELY(*c != '\r')) THROW_RESP_ERROR(client->id);

  TAKE_BYTES(c, 1, false);
  if (*c != '\n') THROW_RESP_ERROR(client->id);

  arg->value = arena_alloc(arena, (arg->len + 1) * sizeof(char));
  if (VERY_UNLIKELY(arg->value == NULL)) THROW_RESP_ERROR(client->id);

  char *argument_raw;
  TAKE_BYTES(argument_raw, arg->len, false);
  memcpy_aligned(arg->value, argument_raw, arg->len);
  arg->value[arg->len] = '\0';

  if (!check_crlf(client, buf, at, size)) THROW_RESP_ERROR(client->id);
  return true;
}

bool parse_resp_command(Arena *arena, Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command) {
  command->args.count = 0;
  command->args.data = NULL;

  char *c;
  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(!('0' <= *c && *c <= '9'))) THROW_RESP_ERROR(client->id);

  do {
    command->args.count = (command->args.count * 10) + (*c - '0');
    TAKE_BYTES(c, 1, false);
  } while ('0' <= *c && *c <= '9');

  if (VERY_UNLIKELY(*c != '\r')) THROW_RESP_ERROR(client->id);

  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(*c != '\n')) THROW_RESP_ERROR(client->id);

  if (command->args.count == 0) {
    write_log(LOG_ERR, "Received data from Client #%u is empty RESP data, so it cannot be created as a command.", client->id);
    return false;
  }

  command->name = arena_alloc(arena, sizeof(string_t));
  command->name->len = 0;
  if (!get_resp_command_name(arena, client, command->name, buf, at, size)) return false;

  command->args.count -= 1;

  if (command->args.count != 0) {
    command->args.data = arena_alloc(arena, command->args.count * sizeof(string_t));

    for (uint32_t i = 0; i < command->args.count; ++i) {
      command->args.data[i].len = 0;
      command->args.data[i].value = NULL;

      const bool parsed = get_resp_command_argument(arena, client, &command->args.data[i], buf, at, size);
      if (VERY_UNLIKELY(!parsed)) return false;
    }
  }

  return true;
}
