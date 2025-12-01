#include <telly.h>

#include <stdbool.h>
#include <stdint.h>

extern bool check_crlf(struct Client *client, char *buf, int32_t *at, int32_t *size);

static inline bool get_resp_command_name(Arena *arena, struct Client *client, string_t *name, char *buf, int32_t *at, int32_t *size) {
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

static inline bool get_resp_command_argument(struct Client *client, Arena *arena, string_t *argument, char *buf, int32_t *at, int32_t *size) {
  char *c;
  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(*c != RDT_BSTRING)) THROW_RESP_ERROR(client->id);

  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(!('0' <= *c && *c <= '9'))) THROW_RESP_ERROR(client->id);

  do {
    argument->len = (argument->len * 10) + (*c - '0');
    TAKE_BYTES(c, 1, false);
  } while ('0' <= *c && *c <= '9');

  if (VERY_UNLIKELY(*c != '\r')) THROW_RESP_ERROR(client->id);

  TAKE_BYTES(c, 1, false);
  if (*c != '\n') THROW_RESP_ERROR(client->id);

  argument->value = arena_alloc(arena, (argument->len + 1) * sizeof(char));
  if (VERY_UNLIKELY(argument->value == NULL)) THROW_RESP_ERROR(client->id);

  char *argument_raw;
  TAKE_BYTES(argument_raw, argument->len, false);
  memcpy_aligned(argument->value, argument_raw, argument->len);
  argument->value[argument->len] = '\0';

  if (!check_crlf(client, buf, at, size)) THROW_RESP_ERROR(client->id);
  return true;
}

bool parse_resp_command(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command) {
  command->arena = arena_create(RESP_ARENA_SIZE);
  command->arg_count = 0;
  command->args = NULL;

  char *c;
  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(!('0' <= *c && *c <= '9'))) THROW_RESP_ERROR(client->id);

  do {
    command->arg_count = (command->arg_count * 10) + (*c - '0');
    TAKE_BYTES(c, 1, false);
  } while ('0' <= *c && *c <= '9');

  if (VERY_UNLIKELY(*c != '\r')) THROW_RESP_ERROR(client->id);

  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(*c != '\n')) THROW_RESP_ERROR(client->id);

  if (command->arg_count == 0) {
    write_log(LOG_ERR, "Received data from Client #%u is empty RESP data, so it cannot be created as a command.", client->id);
    return false;
  }

  command->name = arena_alloc(command->arena, sizeof(string_t));
  command->name->len = 0;
  if (!get_resp_command_name(command->arena, client, command->name, buf, at, size)) return false;

  command->arg_count -= 1;

  if (command->arg_count != 0) {
    command->args = arena_alloc(command->arena, command->arg_count * sizeof(string_t));

    for (uint32_t i = 0; i < command->arg_count; ++i) {
      command->args[i].len = 0;
      command->args[i].value = NULL;

      if (VERY_UNLIKELY(!get_resp_command_argument(client, command->arena, &command->args[i], buf, at, size))) {
        return false;
      }
    }
  }

  return true;
}
