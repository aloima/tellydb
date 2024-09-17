#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>

#include <unistd.h>

static void run(struct Client *client, respdata_t *data, struct Configuration *conf) {
  if (client && data->count != 3) {
    write(client->connfd, "-Wrong argument count for 'HGET' command\r\n", 42);
    return;
  }

  string_t key = data->value.array[1]->value.string;
  struct KVPair *pair = get_data(key.value, conf);

  if (pair && pair->type == TELLY_HASHTABLE) {
    char *name = data->value.array[2]->value.string.value;
    struct FVPair *field = get_fv_from_hashtable(pair->value.hashtable, name);

    if (field) {
      switch (field->type) {
        case TELLY_NULL:
          write(client->connfd, "$-1\r\n", 5);
          break;

        case TELLY_INT: {
          const uint32_t digit_count = get_digit_count(field->value.integer);
          const uint32_t buf_len = get_digit_count(digit_count) + digit_count + 5;

          char buf[buf_len + 1];
          sprintf(buf, "$%d\r\n%d\r\n", digit_count, field->value.integer);

          write(client->connfd, buf, buf_len);
          break;
        }

        case TELLY_STR: {
          const uint32_t buf_len = get_digit_count(field->value.string.len) + field->value.string.len + 5;

          char buf[buf_len + 1];
          sprintf(buf, "$%ld\r\n%s\r\n", field->value.string.len, field->value.string.value);

          write(client->connfd, buf, buf_len);
          break;
        }

        case TELLY_BOOL:
          if (field->value.boolean) {
            write(client->connfd, "$4\r\ntrue\r\n", 10);
          } else {
            write(client->connfd, "$5\r\nfalse\r\n", 11);
          }

          break;

        default:
          break;
      }
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
