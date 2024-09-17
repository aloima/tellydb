#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (client) {
    if (data->count != 2) {
      write(client->connfd, "-Wrong argument count for 'HLEN' command\r\n", 42);
      return;
    }

    char *key = data->value.array[1]->value.string.value;
    struct KVPair *kv = get_data(key, conf);
    uint64_t count = (kv && kv->type == TELLY_HASHTABLE) ? kv->value.hashtable->count : 0;

    uint32_t buf_len = 3 + get_digit_count(count);
    char buf[buf_len + 1];
    sprintf(buf, ":%ld\r\n", count);
    write(client->connfd, buf, buf_len);
  }
}

struct Command cmd_hlen = {
  .name = "HLEN",
  .summary = "Returns field count of the hash table for the key.",
  .since = "1.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
