#include <telly.h>
#include "resp.h"

#define PARSE_COMMAND(type) do {                                                  \
  if (VERY_LIKELY(type[0] == *RDT_ARRAY)) {                                       \
    return parse_resp_command(arena, client, buf, at, size, command);             \
  } else if (VERY_LIKELY(isalnum(type[0]))) {                                     \
    return parse_inline_command(arena, client, buf, at, size, command, *type);    \
  }                                                                               \
} while (0)

bool get_command_data(Arena *arena, Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command) {
  char *type;
  TAKE_BYTES(type, 1, false);

  PARSE_COMMAND(type);

  // Passing leading CRLFs for compatibility on `valkey-cli --pipe` command
  // https://github.com/aloima/tellydb/issues/44
  if (VERY_UNLIKELY(type[0] == '\r')) {
    char *lf;
    TAKE_BYTES(lf, 1, false);

    if (lf[0] != '\n') return false;

    char *cr;

    while (true) {
      TAKE_BYTES(cr, 1, false);
      if (cr[0] != '\r') break;

      TAKE_BYTES(lf, 1, false);
      if (lf[0] != '\n') return false;
    }

    PARSE_COMMAND(cr);
  }

  return false;
}
