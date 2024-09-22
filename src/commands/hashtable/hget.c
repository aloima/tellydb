#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (client && data->count != 3) {
    WRONG_ARGUMENT_ERROR(client->connfd, "HGET", 4);
    return;
  }

  string_t key = data->value.array[1]->value.string;
  struct KVPair *pair = get_data(key.value, conf);

  if (pair && pair->type == TELLY_HASHTABLE) {
    char *name = data->value.array[2]->value.string.value;
    struct FVPair *field = get_fv_from_hashtable(pair->value.hashtable, name);

    if (field) {
      write_value(client->connfd, field->value, field->type);
    } else {
      write(client->connfd, "$-1\r\n", 5);
    }
  } else if (client) {
    write(client->connfd, "$-1\r\n", 5);
  }
}

struct Command cmd_hget = {
  .name = "HGET",
  .summary = "Gets a field from the hash table for the key.",
  .since = "1.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
