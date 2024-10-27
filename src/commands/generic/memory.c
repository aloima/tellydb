#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/utils.h"

#include <stdio.h>
#include <stdint.h>

static void run(struct Client *client, commanddata_t *command) {
  if (client) {
    if (command->arg_count != 0) {
      const string_t subcommand_string = command->args[0];
      char subcommand[subcommand_string.len + 1];
      to_uppercase(subcommand_string.value, subcommand);

      if (streq(subcommand, "USAGE")) {
        if (command->arg_count == 2) {
          const char *key = command->args[1].value;
          struct KVPair *kv = get_kv_from_cache(key);

          if (kv) {
            const uint32_t size = 5;/*sizeof(struct KVPair *) + sizeof(struct KVPair) +
              (kv->key.len + 1) + (kv->type == TELLY_STR ? (kv->value->string.len + 1) : 0);*/

            const uint32_t buf_len = 3 + get_digit_count(size);
            char buf[buf_len + 1];
            sprintf(buf, ":%d\r\n", size);

            _write(client, buf, buf_len);
          } else WRITE_NULL_REPLY(client);
        } else {
          WRONG_ARGUMENT_ERROR(client, "MEMORY USAGE", 12);
        }
      } else {
        WRONG_ARGUMENT_ERROR(client, "MEMORY", 6);
      }
    } else {
      WRONG_ARGUMENT_ERROR(client, "MEMORY", 6);
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
