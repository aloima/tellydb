#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stdint.h>
#include <time.h>

static void run(struct Client *client, __attribute__((unused)) commanddata_t *command) {
  uint64_t age;
  time_t start_at;
  get_server_time(&start_at, &age);
  age += difftime(time(NULL), start_at);

  save_data(age);
  if (client) _write(client, "+OK\r\n", 5);
}

struct Command cmd_save = {
  .name = "SAVE",
  .summary = "Saves all data to database file.",
  .since = "0.1.6",
  .complexity = "O(N) where N is cached key-value pairs",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
