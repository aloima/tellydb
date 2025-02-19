#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>

#include <sys/stat.h>

static void run(struct CommandEntry entry) {
  if (entry.client) {
    if (entry.password->permissions & P_SERVER) {
      const struct Configuration *conf = get_server_configuration();
      struct stat res;

      if (stat(conf->data_file, &res) == -1) {
        write_log(LOG_ERR, "stat(): Cannot access database file");
        return;
      }

      const uint64_t last_save = res.st_mtime;
      char buf[24];
      const size_t nbytes = sprintf(buf, ":%ld\r\n", last_save);

      _write(entry.client, buf, nbytes);
    } else {
      _write(entry.client, "-Not allowed to use this command, need P_SERVER\r\n", 49);
    }
  }
}

const struct Command cmd_lastsave = {
  .name = "LASTSAVE",
  .summary = "Returns last save time of database as UNIX time.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
