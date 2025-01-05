#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/utils.h"

#include <stdint.h>
#include <stdlib.h>

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (command->arg_count < 2) {
    if (client) WRONG_ARGUMENT_ERROR(client, "SET", 3);
    return;
  }

  if (password->permissions & P_WRITE) {
    char *value_in = command->args[1].value;
    bool get = false;
    bool nx = false, xx = false;

    for (uint32_t i = 2; i < command->arg_count; ++i) {
      string_t input = command->args[i];
      char arg[input.len + 1];
      to_uppercase(input.value, arg);

      if (streq(arg, "GET")) get = true;
      else if (streq(arg, "NX")) nx = true;
      else if (streq(arg, "XX")) xx = true;
      else {
        if (client) _write(client, "-Invalid argument(s) for 'SET' command\r\n", 40);
        return;
      }
    }

    if (nx && xx) {
      if (client) _write(client, "-XX and NX arguments cannot be specified simultaneously for 'SET' command\r\n", 75);
      return;
    }

    const string_t key = command->args[0];
    void *value;
    enum TellyTypes type;
    struct KVPair *res = get_data(key);

    if (nx && res) {
      if (client) WRITE_NULL_REPLY(client);
      return;
    }

    if (xx && !res) {
      if (client) WRITE_NULL_REPLY(client);
      return;
    }

    bool is_true = streq(value_in, "true");

    if (is_integer(value_in)) {
      const long number = atol(value_in);
      type = TELLY_NUM;
      value = malloc(sizeof(long));
      *((long *) value) = number;
    } else if (is_true || streq(value_in, "false")) {
      type = TELLY_BOOL;
      value = malloc(sizeof(bool));
      *((bool *) value) = is_true;
    } else if (streq(value_in, "null")) {
      type = TELLY_NULL;
      value = NULL;
    } else {
      const string_t _value = command->args[1];
      type = TELLY_STR;

      string_t *string = (value = malloc(sizeof(string_t)));
      string->len = _value.len;
      string->value = malloc(string->len);
      memcpy(string->value, _value.value, string->len);
    }

    if (get) {
      if (password->permissions & P_READ) {
        if (res) {
          if (client) write_value(client, value, type);
        } else if (client) WRITE_NULL_REPLY(client);

        set_data(res, key, value, type);
      } else {
        _write(client, "-Not allowed to use this command, need P_READ\r\n", 47);
      }
    } else {
      set_data(res, key, value, type);
      if (client) WRITE_OK(client);
    }
  } else {
    _write(client, "-Not allowed to use this command, need P_WRITE\r\n", 48);
  }
}

const struct Command cmd_set = {
  .name = "SET",
  .summary = "Sets value.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
