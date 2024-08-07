#include "../telly.h"

void execute_commands(int connfd, respdata_t data) {
  if (data.type == RDT_ARRAY) {
    char *command = data.value.array[0].value.string.data;

    if (streq(command, "COMMAND")) {
      cmd_command(connfd, data);
    } else if (streq(command, "GET")) {
      cmd_get(connfd, data);
    }
  } else {
    client_error();
  }
}
