#include <telly.h>
#include "resp.h"

extern bool check_crlf(Client *client, int32_t *at, int32_t *size);

static inline bool parse_name(Client *client, int32_t *at, int32_t *size, commanddata_t *cmd, char *c) {
  if (*c == ' ')
    return false;

  uint32_t start_offset = *at - 1;
  uint32_t len = 1;

  while (true) {
    if ((*at + 2) == *size) break;

    char *next_c;
    if (take_n_bytes(client, at, &next_c, 1, size) != 1) {
      break;
    }

    if (*next_c == ' ') break;
    len += 1;
  }

  cmd->name.value = (char *) (uintptr_t) start_offset;
  cmd->name.len = len;

  if ((*at + 2) == *size)
    return check_crlf(client, at, size);
  else
    return true;
}

static inline bool parse_arguments(Client *client, int32_t *at, int32_t *size, commanddata_t *cmd) {
  cmd->args.data = malloc(RESP_INLINE_ARGUMENT_COUNT * sizeof(string_t));
  if (cmd->args.data == NULL)
    return false;

  bool retrieving = true;

  while (retrieving) {
    string_t *arg = &cmd->args.data[cmd->args.count];
    arg->len = 0;
    cmd->args.count += 1;

    char *next_c;
    TAKE_BYTES(next_c, 1, false);

    uint32_t start_offset = *at - 1;
    uint32_t len = 1;

    while (*next_c != ' ') {
      if ((*at + 2) == *size)
        break;

      TAKE_BYTES(next_c, 1, false);

      if (*next_c == ' ')
        break;

      len += 1;
    }

    arg->value = (char *) (uintptr_t) start_offset;
    arg->len = len;

    if ((*at + 2) == *size) {
      if (check_crlf(client, at, size)) break;
      return false;
    }
  }

  return true;
}

bool parse_inline_command(Client *client, int32_t *at, int32_t *size, commanddata_t *cmd, char c) {
  cmd->args.data = NULL;
  cmd->args.count = 0;
  cmd->name.len = 0;

  if (!parse_name(client, at, size, cmd, &c))
    THROW_RESP_ERROR(client->id);

  if (*at != *size) {
    if (!parse_arguments(client, at, size, cmd))
      THROW_RESP_ERROR(client->id);
  }

  // Resolve offsets
  cmd->name.value = client->read_buf->data + (uintptr_t) cmd->name.value;
  cmd->name.value[cmd->name.len] = '\0';

  for (uint32_t i = 0; i < cmd->args.count; ++i) {
    cmd->args.data[i].value = client->read_buf->data + (uintptr_t) cmd->args.data[i].value;
    cmd->args.data[i].value[cmd->args.data[i].len] = '\0';
  }

  return true;
}
