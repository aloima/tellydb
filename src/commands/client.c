#include "../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, [[maybe_unused]] struct Configuration *conf) {
  if (data->count != 1 && client != NULL) {
    char *subcommand = data->value.array[1].value.string.value;

    if (streq("ID", subcommand)) {
      const uint32_t len = (4 + (uint32_t) log10(client->id));
      char res[len + 1];
      sprintf(res, ":%d\r\n", client->id);

      write(client->connfd, res, len);
    } else if (streq("INFO", subcommand)) {
      char buf[512];
      sprintf(buf, (
        "id: %d\r\n"
        "socket file descriptor: %d\r\n"
        "connected at: %.24s\r\n"
        "last used command: %s\r\n"
      ), client->id, client->connfd, ctime(&client->connected_at), client->command->name);

      char res[1024];
      sprintf(res, "$%ld\r\n%s\r\n", strlen(buf), buf);

      write(client->connfd, res, strlen(res));
    }
  }
}

struct Command cmd_client = {
  .name = "CLIENT",
  .summary = "Gives information about client(s).",
  .run = run
};
