#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stdio.h>
#include <stdint.h>

#include <sys/time.h>
#include <unistd.h>

static void run(struct Client *client, respdata_t *data) {
  if (client) {
    if (data->count != 1) {
      WRONG_ARGUMENT_ERROR(client->connfd, "TIME", 4);
      return;
    }

    struct timeval timestamp;
    gettimeofday(&timestamp, NULL);

    const uint32_t sec_len = get_digit_count(timestamp.tv_sec);
    const uint32_t usec_len = get_digit_count(timestamp.tv_usec);

    const uint32_t buf_len = 10 + sec_len + usec_len;
    char buf[buf_len + 1];
    sprintf(buf, "*2\r\n:%ld\r\n:%ld\r\n", timestamp.tv_sec, timestamp.tv_usec);

    write(client->connfd, buf, buf_len);
  }
}

struct Command cmd_time = {
  .name = "TIME",
  .summary = "Returns the current server time as two elements in a array, a Unix timestamp and microseconds already elapsed in the current second.",
  .since = "0.1.2",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
