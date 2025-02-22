#include "../../../headers/telly.h"

#include <stdint.h>
#include <time.h>

static void run(struct CommandEntry entry) {
  uint64_t server_age;
  time_t start_at;
  get_server_time(&start_at, &server_age);
  server_age += difftime(time(NULL), start_at);

  save_data(server_age);
  if (entry.client) WRITE_OK(entry.client);
}

const struct Command cmd_save = {
  .name = "SAVE",
  .summary = "Saves all data to database file.",
  .since = "0.1.6",
  .complexity = "O(N) where N is cached key-value pair count",
  .permissions = P_SERVER,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
