#include "../../../headers/telly.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

static void run(struct CommandEntry entry) {
  if (entry.data->arg_count < 2) {
    if (entry.client) WRONG_ARGUMENT_ERROR(entry.client, "SET", 3);
    return;
  }

  char *value_in = entry.data->args[1].value;
  bool get = false;
  bool nx = false, xx = false;

  for (uint32_t i = 2; i < entry.data->arg_count; ++i) {
    string_t input = entry.data->args[i];
    char *arg = malloc(input.len + 1);
    to_uppercase(input.value, arg);

    if (streq(arg, "GET")) get = true;
    else if (streq(arg, "NX")) nx = true;
    else if (streq(arg, "XX")) xx = true;
    else {
      if (entry.client) _write(entry.client, "-Invalid argument(s) for 'SET' command\r\n", 40);
      free(arg);
      return;
    }

    free(arg);
  }

  if (nx && xx) {
    if (entry.client) _write(entry.client, "-XX and NX arguments cannot be specified simultaneously for 'SET' command\r\n", 75);
    return;
  }

  const string_t key = entry.data->args[0];
  void *value;
  enum TellyTypes type;
  struct KVPair *res = get_data(entry.database, key);

  if (nx && res) {
    if (entry.client) WRITE_NULL_REPLY(entry.client);
    return;
  }

  if (xx && !res) {
    if (entry.client) WRITE_NULL_REPLY(entry.client);
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
    const string_t _value = entry.data->args[1];
    type = TELLY_STR;

    string_t *string = (value = malloc(sizeof(string_t)));
    string->len = _value.len;
    string->value = malloc(string->len);
    memcpy(string->value, _value.value, string->len);
  }

  if (get) {
    if (entry.password->permissions & P_READ) {
      if (res) {
        if (entry.client) write_value(entry.client, value, type);
      } else if (entry.client) WRITE_NULL_REPLY(entry.client);

      set_data(entry.database, res, key, value, type);
    } else {
      _write(entry.client, "-Not allowed to use this command, need P_READ\r\n", 47);
    }
  } else {
    const bool success = set_data(entry.database, res, key, value, type);
    if (!entry.client) return;

    if (success) WRITE_OK(entry.client);
    else WRITE_ERROR(entry.client);
  }
}

const struct Command cmd_set = {
  .name = "SET",
  .summary = "Sets value.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .permissions = P_WRITE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
