#include "../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration conf) {
  if (data->count != 1 && client != NULL) {
    char *subcommand = data->value.array[1].value.string.value;

    if (streq("ID", subcommand)) {
      const uint32_t len = (4 + (uint32_t) log10(client->id));
      char res[len + 1];
      sprintf(res, ":%d\r\n", client->id);

      write(client->connfd, res, len);
    }
  }
}

struct Command cmd_client = {
  .name = "CLIENT",
  .summary = "Gives information about client(s).",
  .run = run
};
