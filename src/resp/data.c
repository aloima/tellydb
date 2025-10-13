#include <telly.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

bool get_command_data(struct Client *client, char *buf, int32_t *at, int32_t *size, commanddata_t *command) {
  uint8_t type;
  TAKE_BYTES(&type, 1, false);

  if (VERY_LIKELY(type == RDT_ARRAY)) {
    return parse_resp_command(client, buf, at, size, command);
  } else {
    write_log(LOG_ERR, "Received data from Client #%u is not RESP array, so it cannot be read as a command.", client->id);
    return false;
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
