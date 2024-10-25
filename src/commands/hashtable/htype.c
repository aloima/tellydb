#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/hashtable.h"
#include "../../../headers/utils.h"

#include <stddef.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count != 3) {
      WRONG_ARGUMENT_ERROR(client, "HTYPE", 5);
      return;
    }

    const char *key = data->value.array[1]->value.string.value;
    char *name = data->value.array[2]->value.string.value;

    const struct KVPair *kv = get_data(key);

    if (kv && kv->type == TELLY_HASHTABLE) {
      struct FVPair *fv = get_fv_from_hashtable(kv->value, name);

      switch (fv->type) {
        case TELLY_NULL:
          _write(client, "+null\r\n", 7);
          break;

        case TELLY_NUM:
          _write(client, "+number\r\n", 9);
          break;

        case TELLY_STR:
          _write(client, "+string\r\n", 9);
          break;

        case TELLY_BOOL:
          _write(client, "+boolean\r\n", 10);
          break;

        default:
          break;
      }
    } else WRITE_NULL_REPLY(client);
  }
}

struct Command cmd_htype = {
  .name = "HTYPE",
  .summary = "Returns type of field from hash table stored at key.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
