#include "../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (client != NULL) {
    switch (data->count) {
      case 1:
        write(client->connfd, "-missing argument\r\n", 19);
        break;

      case 2: {
        char *key = data->value.array[1]->value.string.value;
        struct KVPair *result = get_data(key, conf);

        if (result == NULL) {
          write(client->connfd, "$-1\r\n", 5);
        } else {
          switch (result->type) {
            case TELLY_NULL:
              write(client->connfd, "$-1\r\n", 5);
              break;

            case TELLY_INT: {
              const uint32_t digit_count = get_digit_count(result->value.integer);
              const uint32_t buf_len = get_digit_count(digit_count) + digit_count + 5;

              char buf[buf_len + 1];
              sprintf(buf, "$%d\r\n%d\r\n", digit_count, result->value.integer);

              write(client->connfd, buf, buf_len);
              break;
            }

            case TELLY_STR: {
              const uint32_t buf_len = get_digit_count(result->value.string.len) + result->value.string.len + 5;

              char buf[buf_len + 1];
              sprintf(buf, "$%ld\r\n%s\r\n", result->value.string.len, result->value.string.value);

              write(client->connfd, buf, buf_len);
              break;
            }

            case TELLY_BOOL:
              if (result->value.boolean) {
                write(client->connfd, "$4\r\ntrue\r\n", 10);
              } else {
                write(client->connfd, "$5\r\nfalse\r\n", 11);
              }

              break;

            default:
              break;
          }
        }

        break;
      }

      default:
        write(client->connfd, "-additional argument(s)\r\n", 25);
        break;
    }
  }
}

struct Command cmd_get = {
  .name = "GET",
  .summary = "Gets value from specified key.",
  .since = "1.0.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
