#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, __attribute__((unused)) struct Configuration *conf) {
  if (client) {
    if (data->count != 1) {
      string_t subcommand_string = data->value.array[1]->value.string;
      char subcommand[subcommand_string.len + 1];
      to_uppercase(subcommand_string.value, subcommand);

      if (streq(subcommand, "USAGE")) {
        if (data->count == 3) {
          char *key = data->value.array[2]->value.string.value;
          struct KVPair *pair = get_kv_from_cache(key);

          if (pair) {
            const uint32_t size = sizeof(struct KVPair *) + sizeof(struct KVPair) +
              (pair->key.len + 1) + (pair->type == TELLY_STR ? (pair->value.string.len + 1) : 0);

            const uint32_t buf_len = 3 + get_digit_count(size);
            char buf[buf_len + 1];
            sprintf(buf, ":%d\r\n", size);

            write(client->connfd, buf, buf_len);
          } else {
            write(client->connfd, "$-1\r\n", 5);
          }
        } else {
          WRONG_ARGUMENT_ERROR(client->connfd, "MEMORY USAGE", 12);
        }
      } else {
        WRONG_ARGUMENT_ERROR(client->connfd, "MEMORY", 6);
      }
    } else {
      WRONG_ARGUMENT_ERROR(client->connfd, "MEMORY", 6);
    }
  }
}

static struct Subcommand subcommands[] = {
  (struct Subcommand) {
    .name = "USAGE",
    .summary = "Gives how many bytes are used in the memory for the key.",
    .since = "0.1.2",
    .complexity = "O(1)"
  }
};

struct Command cmd_memory = {
  .name = "MEMORY",
  .summary = "Gives information about the memory.",
  .since = "0.1.2",
  .complexity = "O(1)",
  .subcommands = subcommands,
  .subcommand_count = 1,
  .run = run
};
