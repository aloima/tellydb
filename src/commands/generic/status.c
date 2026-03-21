#include <telly.h>

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  time_t start_at;
  uint32_t age;
  get_server_time(&start_at, &age);

  /* Age from server start + runtime since last snapshot */
  age += difftime(time(NULL), start_at);

  char start_at_buf[21];
  generate_date_string(start_at_buf, start_at);

  char last_error_buf[32];
  if (server->last_error_at != 0) {
    generate_date_string(last_error_buf, server->last_error_at);
  } else {
    sprintf(last_error_buf, "none");
  }

  const char *status_text;
  switch (server->status) {
    case SERVER_STATUS_STARTING: status_text = "starting"; break;
    case SERVER_STATUS_ONLINE: status_text = "online"; break;
    case SERVER_STATUS_ERROR: status_text = "error"; break;
    case SERVER_STATUS_CLOSED: status_text = "closed"; break;
    default: status_text = "unknown"; break;
  }

  char body[1024];
  const int body_len = sprintf(body,
    "# Status\r\n"
    "Version: " VERSION "\r\n"
    "Status: %s\r\n"
    "Started at: %s\r\n"
    "Age: %" PRIu32 " seconds\r\n"
    "Connected clients: %u\r\n"
    "Total processed transactions: %" PRIu64 "\r\n"
    "LastErrorDate: %s\r\n",
    status_text,
    start_at_buf,
    age,
    get_client_count(),
    get_processed_transaction_count(),
    last_error_buf
  );

  const size_t output_len = sprintf(entry->client->write_buf, "$%d\r\n%s\r\n", body_len, body);
  return CREATE_STRING(entry->client->write_buf, output_len);
}

const struct Command cmd_status = {
  .name = "STATUS",
  .summary = "Displays server status including last error date.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .permissions = P_NONE,
  .flags.value = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
