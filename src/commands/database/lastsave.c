#include <telly.h>

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include <sys/stat.h>

static string_t run(struct CommandEntry entry) {
  PASS_NO_CLIENT(entry.client);

  const struct Configuration *conf = get_server_configuration();
  struct stat res;

  if (stat(conf->data_file, &res) == -1) {
    write_log(LOG_ERR, "stat(): Cannot access database file");
    return RESP_ERROR();
  }

  const uint64_t last_save = res.st_mtime;

  const size_t nbytes = sprintf(entry.buffer, ":%" PRIu64 "\r\n", last_save);
  return CREATE_STRING(entry.buffer, nbytes);
}

const struct Command cmd_lastsave = {
  .name = "LASTSAVE",
  .summary = "Returns last save time of database as UNIX time.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .permissions = P_SERVER,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
