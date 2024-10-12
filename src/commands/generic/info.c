#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>

static bool get_section(char *section, const struct Configuration *conf, const char *name) {
  if (streq(name, "server")) {
    char gcc_version[16];
    sprintf(gcc_version, "%d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);

    sprintf(section, (
      "# Server\r\n"
      "Version: " VERSION "\r\n"
      "Process ID: %d\r\n"
      "Git hash: " GIT_HASH "\r\n"
      "Multiplexing API: poll\r\n"
      "GCC version: %s\r\n"
      "TLS server: %s\r\n"
    ), getpid(), gcc_version, conf->tls ? "true" : "false");
  } else if (streq(name, "clients")) {
    sprintf(section, (
      "# Clients\r\n"
      "Connected clients: %d\r\n"
      "Max clients: %d\r\n"
      "Transaction count: %d\r\n"
      "Total connection count: %d\r\n"
    ), get_client_count(), conf->max_clients, get_transaction_count(), get_last_connection_client_id());
  } else {
    return false;
  }

  return true;
}

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    const struct Configuration *conf = get_server_configuration();
    const uint32_t count = data->count - 1;

    char buf[8192], section[2048];
    buf[0] = '\0';

    if (count != 0) {
      const uint32_t n = count - 1;

      for (uint32_t i = 0; i < n; ++i) {
        char *name = data->value.array[i + 1]->value.string.value;

        if (!get_section(section, conf, name)) {
          _write(client, "-Invalid section name\r\n", 23);
          return;
        }

        strcat(buf, section);
        strcat(buf, "\r\n");
      }

      const char *name = data->value.array[count]->value.string.value;

      if (!get_section(section, conf, name)) {
        _write(client, "-Invalid section name\r\n", 23);
        return;
      }

      strcat(buf, section);
    } else {
      const char names[][32] = {"server", "clients"};
      const uint32_t n = 1;

      for (uint32_t i = 0; i < n; ++i) {
        const char *name = names[i];

        if (!get_section(section, conf, name)) {
          _write(client, "-Invalid section name\r\n", 23);
          return;
        }

        strcat(buf, section);
        strcat(buf, "\r\n");
      }

      const char *name = names[n];

      if (!get_section(section, conf, name)) {
        _write(client, "-Invalid section name\r\n", 23);
        return;
      }

      strcat(buf, section);
    }

    const uint32_t buf_len = strlen(buf);
    const uint32_t res_len = buf_len + 5 + get_digit_count(buf_len);
    char res[res_len + 1];

    sprintf(res, "$%d\r\n%s\r\n", buf_len, buf);
    _write(client, res, res_len);
  }
}

struct Command cmd_info = {
  .name = "INFO",
  .summary = "Displays server information.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
