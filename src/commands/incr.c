#include "../../headers/telly.h"

#include <stdio.h>

#include <stdint.h>
#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  const bool is_client = client != NULL;

  if (data->count == 1) {
    if (is_client) {
      write(client->connfd, "-missing argument\r\n", 19);
    }
  } else if (data->count == 2) {
    string_t key = data->value.array[1]->value.string;
    struct KVPair *result = get_data(key.value, conf);

    if (result == NULL) {
      struct KVPair pair = {
        .key = key,
        .type = TELLY_INT,
        .value = {
          .integer = 0
        }
      };

      set_data(pair, conf);

      if (is_client) {
        write(client->connfd, ":0\r\n", 4);
      }
    } else if (result->type == TELLY_INT) {
      result->value.integer += 1;

      if (client != NULL) {
        const uint32_t buf_size = get_digit_count(result->value.integer) + 3;
        char buf[buf_size + 1];

        sprintf(buf, ":%d\r\n", result->value.integer);
        write(client->connfd, buf, buf_size);
      }
    } else if (is_client) {
      write(client->connfd, "-invalid type for INCR command\r\n", 32);
    }
  } else if (is_client) {
    write(client->connfd, "-additional argument(s)\r\n", 25);
  }
}

struct Command cmd_incr = {
  .name = "INCR",
  .summary = "Increments value from specified key.",
  .since = "1.0.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
