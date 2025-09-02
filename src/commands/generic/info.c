#include <telly.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>

#include <unistd.h>

static bool get_section(char *section, const struct Configuration *conf, const char *name) {
  if (streq(name, "server")) {
    char gcc_version[16];
    sprintf(gcc_version, "%d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);

    uint32_t age;
    time_t start_at;
    get_server_time(&start_at, &age);

    age += difftime(time(NULL), start_at);

    char str_start_at[21];
    generate_date_string(str_start_at, start_at);

    sprintf(section, (
      "# Server\r\n"
      "Version: " VERSION "\r\n"
      "Process ID: %d\r\n"
      "Git hash: " GIT_HASH "\r\n"
      "Multiplexing API: epoll\r\n"
      "GCC version: %s\r\n"
      "TLS server: %s\r\n"
      "Age: %u seconds\r\n"
      "Started at: %.20s\r\n"
    ), getpid(), gcc_version, (conf->tls ? "yes" : "no"), age, str_start_at);
  } else if (streq(name, "clients")) {
    sprintf(section, (
      "# Clients\r\n"
      "Connected clients: %u\r\n"
      "Max clients: %u\r\n"
      "Transactions: %u\r\n"
      "Max transaction blocks: %u\r\n"
      "Total processed transactions: %" PRIu64 "\r\n"
      "Total received connections: %u\r\n"
    ), get_client_count(), conf->max_clients, get_transaction_count(), conf->max_transaction_blocks, get_processed_transaction_count(), get_last_connection_client_id());
  } else {
    return false;
  }

  return true;
}

static void run(struct CommandEntry entry) {
  if (!entry.client) return;
  const struct Configuration *conf = get_server_configuration();

  char buf[8192], section[2048];
  buf[0] = '\0';

  if (entry.data->arg_count != 0) {
    const uint32_t n = entry.data->arg_count - 1;

    for (uint32_t i = 0; i < n; ++i) {
      char *name = entry.data->args[i].value;

      if (!get_section(section, conf, name)) {
        WRITE_ERROR_MESSAGE(entry.client, "Invalid section name");
        return;
      }

      strcat(buf, section);
      strcat(buf, "\r\n");
    }

    const char *name = entry.data->args[n].value;

    if (!get_section(section, conf, name)) {
      WRITE_ERROR_MESSAGE(entry.client, "Invalid section name");
      return;
    }

    strcat(buf, section);
  } else {
    const char names[][32] = {"server", "clients"};
    const uint32_t n = 1;

    for (uint32_t i = 0; i < n; ++i) {
      const char *name = names[i];

      if (!get_section(section, conf, name)) {
        WRITE_ERROR_MESSAGE(entry.client, "Invalid section name");
        return;
      }

      strcat(buf, section);
      strcat(buf, "\r\n");
    }

    const char *name = names[n];

    if (!get_section(section, conf, name)) {
      WRITE_ERROR_MESSAGE(entry.client, "Invalid section name");
      return;
    }

    strcat(buf, section);
  }

  const uint16_t buf_len = strlen(buf);
  char res[buf_len + 11];

  const size_t nbytes = sprintf(res, "$%hu\r\n%s\r\n", buf_len, buf);
  _write(entry.client, res, nbytes);
}

const struct Command cmd_info = {
  .name = "INFO",
  .summary = "Displays server information.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
