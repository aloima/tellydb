#include "../../../headers/telly.h"

#include <stdint.h>
#include <time.h>

static void run(struct CommandEntry entry) {
  uint32_t server_age;
  time_t start_at;
  get_server_time(&start_at, &server_age);
  server_age += difftime(time(NULL), start_at);

  if (bg_save(server_age) && entry.client) WRITE_OK(entry.client);
  else if (entry.client) _write(entry.client, "-Saving process is already active in background or not\r\n", 56);
}

const struct Command cmd_bgsave = {
  .name = "BGSAVE",
  .summary = "Saves all data to database file in background using a thread.",
  .since = "0.1.6",
  .complexity = "O(N) where N is cached key-value pair count",
  .permissions = P_SERVER,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
