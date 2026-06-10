#include <telly.h>
#include "resp.h"

extern bool check_crlf(Client *client, int32_t *at, int32_t *size);

static inline bool get_resp_command_name(Client *client, string_t *name, int32_t *at, int32_t *size) {
  uint32_t len = 0;

  char *c;
  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(*c != *RDT_BSTRING))
    THROW_RESP_ERROR(client->id);

  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(!('0' <= *c && *c <= '9')))
    THROW_RESP_ERROR(client->id);

  do {
    len = (len * 10) + (*c - '0');
    TAKE_BYTES(c, 1, false);
  } while ('0' <= *c && *c <= '9');

  if (VERY_UNLIKELY(*c != '\r'))
    THROW_RESP_ERROR(client->id);

  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(*c != '\n'))
    THROW_RESP_ERROR(client->id);

  char *raw;
  TAKE_BYTES(raw, len, false);

  name->value = (char *) ((uintptr_t) (raw - client->read_buf->data));
  name->len = len;

  if (!check_crlf(client, at, size))
    THROW_RESP_ERROR(client->id);

  return true;
}

static inline bool get_resp_command_argument(Client *client, string_t *arg, int32_t *at, int32_t *size) {
  char *c;
  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(*c != *RDT_BSTRING)) THROW_RESP_ERROR(client->id);

  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(!('0' <= *c && *c <= '9'))) THROW_RESP_ERROR(client->id);

  do {
    arg->len = (arg->len * 10) + (*c - '0');
    TAKE_BYTES(c, 1, false);
  } while ('0' <= *c && *c <= '9');

  if (VERY_UNLIKELY(*c != '\r')) THROW_RESP_ERROR(client->id);

  TAKE_BYTES(c, 1, false);
  if (*c != '\n') THROW_RESP_ERROR(client->id);

  char *argument_raw;
  TAKE_BYTES(argument_raw, arg->len, false);

  arg->value = (char *) (uintptr_t) (argument_raw - client->read_buf->data);

  if (!check_crlf(client, at, size))
    THROW_RESP_ERROR(client->id);

  return true;
}

bool parse_resp_command(Client *client, int32_t *at, int32_t *size, commanddata_t *command) {
  command->args.count = 0;
  command->args.data = NULL;
  command->name.len = 0;

  char *c;
  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(!('0' <= *c && *c <= '9')))
    THROW_RESP_ERROR(client->id);

  do {
    command->args.count = (command->args.count * 10) + (*c - '0');
    TAKE_BYTES(c, 1, false);
  } while ('0' <= *c && *c <= '9');

  if (VERY_UNLIKELY(*c != '\r'))
    THROW_RESP_ERROR(client->id);

  TAKE_BYTES(c, 1, false);
  if (VERY_UNLIKELY(*c != '\n'))
    THROW_RESP_ERROR(client->id);

  if (command->args.count == 0) {
    write_log(LOG_ERR, "Received data from Client #%u is empty RESP data, so it cannot be created as a command.", client->id);
    return false;
  }

  if (!get_resp_command_name(client, &command->name, at, size))
    return false;

  command->args.count -= 1;

  if (command->args.count != 0) {
    command->args.data = malloc(command->args.count * sizeof(string_t));
    if (command->args.data == NULL)
      return false;

    for (uint32_t i = 0; i < command->args.count; ++i) {
      command->args.data[i].len = 0;

      const bool parsed = get_resp_command_argument(client, &command->args.data[i], at, size);
      if (VERY_UNLIKELY(!parsed)) {
        free(command->args.data);
        return false;
      }
    }
  }

  command->name.value = client->read_buf->data + (uintptr_t) command->name.value;

  for (uint32_t i = 0; i < command->args.count; ++i) {
    command->args.data[i].value = client->read_buf->data + (uintptr_t) command->args.data[i].value;
  }

  return true;
}
