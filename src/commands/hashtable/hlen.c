#include <telly.h>

#include <stdio.h>

static void run(struct CommandEntry entry) {
  if (!entry.client) return;
  if (entry.data->arg_count != 1) {
    WRONG_ARGUMENT_ERROR(entry.client, "HLEN");
    return;
  }

  const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

  if (!kv) {
    WRITE_NULL_REPLY(entry.client);
    return;
  }

  if (kv->type != TELLY_HASHTABLE) {
    INVALID_TYPE_ERROR(entry.client, "HLEN");
    return;
  }

  const struct HashTable *table = kv->value;

  char buf[90];
  const size_t nbytes = sprintf(buf, (
    "*3\r\n"
      "+Allocated: %u\r\n"
      "+Filled: %u\r\n"
      "+All (includes next count): %u\r\n"
  ), table->size.allocated, table->size.filled, table->size.all);

  _write(entry.client, buf, nbytes);
}

const struct Command cmd_hlen = {
  .name = "HLEN",
  .summary = "Returns field count information of the hash table.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .permissions = P_READ,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
