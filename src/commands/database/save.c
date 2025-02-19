#include "../../../headers/telly.h"

#include <stdint.h>
#include <time.h>

static void run(struct CommandEntry entry) {
  if (entry.password->permissions & P_SERVER) {
    uint64_t server_age;
    time_t start_at;
    get_server_time(&start_at, &server_age);
    server_age += difftime(time(NULL), start_at);

    save_data(server_age);
    if (entry.client) WRITE_OK(entry.client);
  } else if (entry.client) {
    _write(entry.client, "-Not allowed to use this command, need P_SERVER\r\n", 49);
  }
}

const struct Command cmd_save = {
  .name = "SAVE",
  .summary = "Saves all data to database file.",
  .since = "0.1.6",
  .complexity = "O(N) where N is cached key-value pair count",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
