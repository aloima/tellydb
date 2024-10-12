#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/hashtable.h"

#include <stdio.h>
#include <stdint.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count != 2) {
      WRONG_ARGUMENT_ERROR(client, "HLEN", 4);
      return;
    }

    const char *key = data->value.array[1]->value.string.value;
    const struct KVPair *kv = get_data(key);
    const uint64_t count = (kv && kv->type == TELLY_HASHTABLE) ? kv->value->hashtable->size.all : 0;

    const uint32_t buf_len = 3 + get_digit_count(count);
    char buf[buf_len + 1];
    sprintf(buf, ":%ld\r\n", count);
    _write(client, buf, buf_len);
  }
}

struct Command cmd_hlen = {
  .name = "HLEN",
  .summary = "Returns field count of the hash table for the key.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
