#include <telly.h>

#include <stdio.h>

static string_t run(struct CommandEntry entry) {
  PASS_NO_CLIENT(entry.client);

  if (entry.data->arg_count != 1) {
    return WRONG_ARGUMENT_ERROR("HLEN");
  }

  const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

  if (!kv) {
    return RESP_NULL(entry.client->protover);
  }

  if (kv->type != TELLY_HASHTABLE) {
    return INVALID_TYPE_ERROR("HLEN");
  }

  const struct HashTable *table = kv->value;

  const size_t nbytes = sprintf(entry.buffer, (
    "*3\r\n"
      "+Capacity: %u\r\n"
      "+Used: %u\r\n"
  ), table->size.capacity, table->size.used);

  return CREATE_STRING(entry.buffer, nbytes);
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
