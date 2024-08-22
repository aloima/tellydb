#include "../../headers/telly.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (client != NULL) {
    char buf[8192];
    sprintf(buf, (
      "# Clients\r\n"
      "Connected clients: %d\r\n"
      "Max clients: %d\r\n"
      "Transaction count: %d\r\n"
      "Total connection count: %d\r\n"
    ), get_client_count(), conf->max_clients, get_transaction_count(), get_last_connection_client_id());

    const uint32_t buf_len = strlen(buf);
    const uint32_t res_len = buf_len + 6 + (int32_t) log10(buf_len);
    char res[res_len + 1];

    sprintf(res, "$%d\r\n%s\r\n", buf_len, buf);
    write(client->connfd, res, res_len);
  }
}

struct Command cmd_info = {
  .name = "INFO",
  .summary = "Displays server information.",
  .run = run
};
