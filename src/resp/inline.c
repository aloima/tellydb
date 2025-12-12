#include <telly.h>

#include <stdint.h>
#include <stdbool.h>

extern bool check_crlf(Client *client, char *buf, int32_t *at, int32_t *size);

static inline bool parse_name(Arena *arena, Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *cmd, char *c) {
  if (*c == ' ') return false;
  uint8_t idx = 0;

  cmd->name = arena_alloc(arena, sizeof(string_t));
  cmd->name->value = arena_alloc(arena, RESP_INLINE_BUFFER * sizeof(char));

  do {
    cmd->name->value[idx] = *c;
    if ((*at + 2) == *size) {
      idx += 1;
      break;
    }

    if (take_n_bytes_from_socket(client, buf, at, &c, 1, size) != 1) {
      idx += 1;
      cmd->name->value[idx] = '\0';
      cmd->name->len = idx;
      return true;
    }

    idx += 1;
  } while (*c != ' ');

  cmd->name->value[idx] = '\0';
  cmd->name->len = idx;

  if ((*at + 2) == *size) return check_crlf(client, buf, at, size);
  else return true;
}

static inline bool parse_arguments(Arena *arena, Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *cmd, char *c) {
  bool retrieving = true;
  cmd->args.data = arena_alloc(arena, RESP_INLINE_ARGUMENT_COUNT * sizeof(string_t));

  while (retrieving) {
    string_t *arg = &cmd->args.data[cmd->args.count];
    arg->value = arena_alloc(arena, RESP_INLINE_BUFFER * sizeof(char));
    arg->len = 0;
    cmd->args.count += 1;

    uint8_t idx = 0;
    char *value = arg->value;
    TAKE_BYTES(c, 1, false);

    while (*c != ' ') {
      value[idx] = *c;
      idx += 1;

      if ((*at + 2) == *size) break;
      TAKE_BYTES(c, 1, false);
    }

    if (idx != 0) {
      arg->value[idx] = '\0';
      arg->len += idx;
    }

    if ((*at + 2) == *size) {
      if (check_crlf(client, buf, at, size)) break;
      return false;
    }
  }

  return true;
}

bool parse_inline_command(Arena *arena, Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *cmd, char c) {
  cmd->args.data = NULL;
  cmd->args.count = 0;

  if (!parse_name(arena, client, buf, at, size, cmd, &c)) THROW_RESP_ERROR(client->id);
  if (*at == *size) return true;

  if (!parse_arguments(arena, client, buf, at, size, cmd, &c)) THROW_RESP_ERROR(client->id);
  return true;
}
