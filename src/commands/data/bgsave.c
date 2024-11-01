#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stdint.h>
#include <time.h>

static void run(struct Client *client, __attribute__((unused)) commanddata_t *command) {
  uint64_t server_age;
  time_t start_at;
  get_server_time(&start_at, &server_age);
  server_age += difftime(time(NULL), start_at);

  if (bg_save(server_age)) {
    if (client) _write(client, "+OK\r\n", 5);
  } else if (client) {
    _write(client, "-Saving process is already active in background or not\r\n", 56);
  }
}

struct Command cmd_bgsave = {
  .name = "BGSAVE",
  .summary = "Saves all data to database file in background using a thread.",
  .since = "0.1.6",
  .complexity = "O(N) where N is cached key-value pairs",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
