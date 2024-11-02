#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"

#include <stddef.h>

static void run(struct Client *client, commanddata_t *command) {
  if (command->arg_count != 2) {
    if (client) WRONG_ARGUMENT_ERROR(client, "APPEND", 6);
    return;
  }

  const string_t key = command->args[0];
  const struct KVPair *kv = get_data(key.value);

  if (kv) {
    if (kv->type == TELLY_STR) {
      const string_t arg = command->args[1];

      string_t *string = kv->value;
      string->value = realloc(string->value, string->len + arg.len);
      memcpy(string->value + string->len, arg.value, arg.len);
      string->len += arg.len;

      if (client) {
        char buf[14];
        const size_t nbytes = sprintf(buf, ":%d\r\n", string->len);
        _write(client, buf, nbytes);
      }
    } else if (client) _write(client, "-Invalid type for 'APPEND' command\r\n", 36);
  } else {
    const string_t arg = command->args[1];

    string_t *string = malloc(sizeof(string_t));
    string->len = arg.len;
    string->value = malloc(string->len);
    memcpy(string->value, arg.value, string->len);
    set_data(NULL, key, string, TELLY_STR);

    if (client) {
      char buf[14];
      const size_t nbytes = sprintf(buf, ":%d\r\n", string->len);
      _write(client, buf, nbytes);
    }
  }
}

struct Command cmd_append = {
  .name = "APPEND",
  .summary = "Appends string to existed value. If key is not exist, creates a new one.",
  .since = "0.1.7",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
