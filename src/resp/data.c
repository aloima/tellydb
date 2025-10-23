#include <telly.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

bool get_command_data(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command) {
  char *type;
  TAKE_BYTES(type, 1, false);

  if (VERY_LIKELY(type[0] == RDT_ARRAY)) {
    return parse_resp_command(client, buf, at, size, command);
  } else {
    return parse_inline_command(client, buf, at, size, command, *type);
  }
}

void free_command_data(commanddata_t command) {
  if (command.arg_count != 0) {
    for (uint32_t i = 0; i < command.arg_count; ++i) {
      free(command.args[i].value);
    }

    free(command.args);
  }
}
