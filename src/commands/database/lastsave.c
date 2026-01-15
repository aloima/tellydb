#include <telly.h>

#include <stdio.h>
#include <stdint.h>

#include <sys/stat.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  struct stat res;

  if (stat(server->conf->data_file, &res) == -1) {
    write_log(LOG_ERR, "stat(): Cannot access database file");
    return RESP_ERROR();
  }

  const uint64_t last_save = res.st_mtime;

  const size_t nbytes = create_resp_integer(entry->client->write_buf, last_save);
  return CREATE_STRING(entry->client->write_buf, nbytes);
}

const struct Command cmd_lastsave = {
  .name = "LASTSAVE",
  .summary = "Returns last save time of database as UNIX time.",
  .since = "0.1.6",
  .complexity = "O(1)",
  .permissions = P_SERVER,
  .flags.value = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
