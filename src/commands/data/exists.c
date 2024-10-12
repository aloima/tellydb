#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count == 1) {
      WRONG_ARGUMENT_ERROR(client, "EXISTS", 6);
      return;
    }

    const uint32_t key_count = data->count - 1;
    uint32_t existed = 0, not_existed = 0;

    char buf[8192];
    buf[0] = '\0';

    for (uint32_t i = 1; i <= key_count; ++i) {
      const char *key = data->value.array[i]->value.string.value;

      if (get_data(key)) {
        existed += 1;
        strcat(buf, "+exists\r\n");
      } else {
        not_existed += 1;
        strcat(buf, "+not exist\r\n");
      }
    }

    const uint32_t arr_count = key_count + 2;
    const uint32_t res_len = 55 + strlen(buf) +
      get_digit_count(arr_count) + get_digit_count(existed) + get_digit_count(not_existed);

    char res[res_len + 1];
    sprintf(res, (
      "*%d\r\n"
        "+existed key count is %d\r\n"
        "+not existed key count is %d\r\n"
        "%s"
    ), key_count + 2, existed, not_existed, buf);

    _write(client, res, res_len);
  }
}

struct Command cmd_exists = {
  .name = "EXISTS",
  .summary = "Checks if specified keys exist or not.",
  .since = "0.1.4",
  .complexity = "O(N) where N is key count",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
