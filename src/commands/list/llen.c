#include <telly.h>

#include <stdio.h>
#include <stdint.h>

static void run(struct CommandEntry entry) {
  if (!entry.client) return;
  if (entry.data->arg_count != 1) {
    WRONG_ARGUMENT_ERROR(entry.client, "LLEN", 4);
    return;
  }

  const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

  if (!kv) {
    _write(entry.client, ":0\r\n", 4);
    return;
  } else if (kv->type != TELLY_LIST) {
    _write(entry.client, "-Value stored at the key is not a list\r\n", 40);
    return;
  }

  char buf[14];
  const size_t nbytes = sprintf(buf, ":%u\r\n", ((struct List *) kv->value)->size);
  _write(entry.client, buf, nbytes);
}

const struct Command cmd_llen = {
  .name = "LLEN",
  .summary = "Returns length of the list.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .permissions = P_READ,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
