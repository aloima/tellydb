#include "../../../headers/telly.h"

#include <stdint.h>
#include <stdio.h>

#include <unistd.h>

static void run(struct Client *client, __attribute__((unused)) respdata_t *data, struct Configuration *conf) {
  if (client) {
    char buf[8192];
    sprintf(buf, (
      "# Clients\r\n"
      "Connected clients: %d\r\n"
      "Max clients: %d\r\n"
      "Transaction count: %d\r\n"
      "Total connection count: %d\r\n"
      "\r\n"
      "# Server\r\n"
      "Version: " VERSION "\r\n"
      "Process ID: %d\r\n"
      "Git hash: " GIT_HASH "\r\n"
    ), get_client_count(), conf->max_clients, get_transaction_count(), get_last_connection_client_id(), getpid());

    const uint32_t buf_len = strlen(buf);
    const uint32_t res_len = buf_len + 5 + get_digit_count(buf_len);
    char res[res_len + 1];

    sprintf(res, "$%d\r\n%s\r\n", buf_len, buf);
    write(client->connfd, res, res_len);
  }
}

struct Command cmd_info = {
  .name = "INFO",
  .summary = "Displays server information.",
  .since = "1.0.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
