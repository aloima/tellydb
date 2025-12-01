#include <telly.h>

#include <stdbool.h>
#include <stdint.h>

bool get_command_data(Arena *arena, Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command) {
  char *type;
  TAKE_BYTES(type, 1, false);

  if (VERY_LIKELY(type[0] == RDT_ARRAY)) {
    return parse_resp_command(arena, client, buf, at, size, command);
  } else {
    return parse_inline_command(arena, client, buf, at, size, command, *type);
  }
}
