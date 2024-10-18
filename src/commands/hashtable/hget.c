#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/hashtable.h"
#include "../../../headers/utils.h"

#include <stddef.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count != 3) {
      WRONG_ARGUMENT_ERROR(client, "HGET", 4);
      return;
    }

    const string_t key = data->value.array[1]->value.string;
    const struct KVPair *kv = get_data(key.value);

    if (kv && kv->type == TELLY_HASHTABLE) {
      char *name = data->value.array[2]->value.string.value;
      struct FVPair *field = get_fv_from_hashtable(kv->value->hashtable, name);

      if (field) {
        write_value(client, field->value, field->type);
      } else {
        _write(client, "$-1\r\n", 5);
      }
    } else {
      _write(client, "$-1\r\n", 5);
    }
  }
}

struct Command cmd_hget = {
  .name = "HGET",
  .summary = "Gets a field from the hash table for the key.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
