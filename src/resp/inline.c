#include <telly.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

extern bool check_crlf(struct Client *client, char *buf, int32_t *at, int32_t *size);

static inline bool parse_name(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command, char *c) {
  if (*c == ' ') return false;

  Arena *arena = command->arena;
  uint8_t idx = 0;

  command->name = arena_alloc(arena, sizeof(string_t));
  command->name->value = arena_alloc(arena, RESP_INLINE_BUFFER * sizeof(char));

  do {
    command->name->value[idx] = *c;
    if ((*at + 2) == *size) {
      idx += 1;
      break;
    }

    if (take_n_bytes_from_socket(client, buf, at, &c, 1, size) != 1) {
      idx += 1;
      command->name->value[idx] = '\0';
      command->name->len = idx;
      return true;
    }

    idx += 1;
  } while (*c != ' ');

  command->name->value[idx] = '\0';
  command->name->len = idx;

  if ((*at + 2) == *size) return check_crlf(client, buf, at, size);
  else return true;
}

static inline bool parse_arguments(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command, char *c) {
  Arena *arena = command->arena;
  bool retrieving = true;
  command->args = arena_alloc(arena, RESP_INLINE_ARGUMENT_COUNT * sizeof(string_t));

  while (retrieving) {
    string_t *arg = &command->args[command->arg_count];
    arg->value = arena_alloc(arena, RESP_INLINE_BUFFER * sizeof(char));
    arg->len = 0;
    command->arg_count += 1;

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

bool parse_inline_command(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command, char c) {
  command->arena = arena_create(RESP_ARENA_SIZE);
  command->args = NULL;
  command->arg_count = 0;

  if (!parse_name(client, buf, at, size, command, &c)) THROW_RESP_ERROR(client->id);
  if (*at == *size) return true;

  if (!parse_arguments(client, buf, at, size, command, &c)) THROW_RESP_ERROR(client->id);
  return true;
}
