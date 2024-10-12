#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/hashtable.h"
#include "../../../headers/utils.h"

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

    if (kv) {
      if (kv->type == TELLY_HASHTABLE) {
        struct HashTable *table = kv->value->hashtable;

        const uint32_t buf_len = 59 + get_digit_count(table->size.allocated) +
          get_digit_count(table->size.filled) + get_digit_count(table->size.all);

        char buf[buf_len + 1];
        sprintf(buf, (
          "*3\r\n"
            "+Allocated: %d\r\n"
            "+Filled: %d\r\n"
            "+All (includes next count): %d\r\n"
        ), table->size.allocated, table->size.filled, table->size.all);

        _write(client, buf, buf_len);
      } else {
        _write(client, "-Invalid type for 'HLEN' command\r\n", 34);
      }
    } else {
      _write(client, "$-1\r\n", 5);
    }
  }
}

struct Command cmd_hlen = {
  .name = "HLEN",
  .summary = "Returns field count information of the hash table for the key.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
