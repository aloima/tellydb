#include <telly.h>

#include <stdint.h>
#include <time.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  uint32_t server_age;
  time_t start_at;
  get_server_time(&start_at, &server_age);
  server_age += difftime(time(NULL), start_at);

  if (save_data(server_age)) {
    return RESP_OK();
  } else {
    return RESP_ERROR();
  }
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
