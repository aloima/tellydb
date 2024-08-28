#include "../../headers/telly.h"

#include <stdio.h>
#include <math.h>

#include <stdint.h>
#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  const bool is_client = client != NULL;

  if (data->count == 1) {
    if (is_client) {
      write(client->connfd, "-missing argument\r\n", 19);
    }
  } else if (data->count == 2) {
    string_t key = data->value.array[1].value.string;
    struct KVPair *result = get_data(key.value, conf);

    if (result == NULL) {
      struct KVPair pair = {
        .key = key,
        .type = TELLY_INT,
        .value = {
          .integer = 0
        }
      };

      set_data(pair);

      if (is_client) {
        write(client->connfd, ":0\r\n", 4);
      }
    } else if (result->type == TELLY_INT) {
      result->value.integer -= 1;

      if (client != NULL) {
        const uint32_t buf_size = log10(result->value.integer) + 4;
        char buf[buf_size];

        sprintf(buf, ":%d\r\n", result->value.integer);
        write(client->connfd, buf, buf_size);
      }
    } else if (is_client) {
      write(client->connfd, "-invalid type for DECR command\r\n", 32);
    }
  } else if (is_client) {
    write(client->connfd, "-additional argument(s)\r\n", 25);
  }
}

struct Command cmd_decr = {
  .name = "DECR",
  .summary = "Decrements value from specified key.",
  .run = run
};
