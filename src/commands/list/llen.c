#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stdio.h>
#include <stdint.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count != 2) {
      WRONG_ARGUMENT_ERROR(client, "LLEN", 4);
      return;
    }

    const char *key = data->value.array[1]->value.string.value;
    const struct KVPair *kv = get_data(key);

    if (!kv) {
      _write(client, ":0\r\n", 4);
      return;
    } else if (kv->type != TELLY_LIST) {
      _write(client, "-Value stored at the key is not a list\r\n", 40);
      return;
    }

    const uint32_t size = kv->value->list->size;
    const uint32_t buf_len = 3 + get_digit_count(kv->value->list->size);
    char buf[buf_len + 1];
    sprintf(buf, ":%d\r\n", size);

    _write(client, buf, buf_len);
  }
}

struct Command cmd_llen = {
  .name = "LLEN",
  .summary = "Returns length of the list stored at the key.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
