#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/utils.h"

#include <stdint.h>
#include <stdlib.h>

static void run(struct Client *client, respdata_t *data) {
  if (data->count < 3) {
    if (client) WRONG_ARGUMENT_ERROR(client, "SET", 3);
    return;
  }

  char *value_in = data->value.array[2]->value.string.value;
  bool get = false;
  bool nx = false, xx = false;

  for (uint32_t i = 3; i < data->count; ++i) {
    string_t input = data->value.array[i]->value.string;
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

  const string_t key = data->value.array[1]->value.string;
  void *value;
  enum TellyTypes type;
  struct KVPair *res = get_data(key.value);

  if (nx && res) {
    if (client) _write(client, "$-1\r\n", 5);
    return;
  }

  if (xx && !res) {
    if (client) _write(client, "$-1\r\n", 5);
    return;
  }

  bool is_true = streq(value_in, "true");

  if (is_integer(value_in)) {
    const long _value = atol(value_in);
    type = TELLY_NUM;
    value = malloc(sizeof(long));
    memcpy(value, &_value, sizeof(long));
  } else if (is_true || streq(value_in, "false")) {
    type = TELLY_BOOL;
    value = malloc(sizeof(bool));
    memset(value, is_true, sizeof(bool));
  } else if (streq(value_in, "null")) {
    type = TELLY_NULL;
    value = NULL;
  } else {
    const string_t _value = data->value.array[2]->value.string;
    type = TELLY_STR;

    string_t *string = (value = malloc(sizeof(string_t)));
    string->len = _value.len;
    string->value = malloc(string->len);
    memcpy(string->value, _value.value, string->len);
  }

  if (get) {
    if (res) {
      if (client) write_value(client, value, type);
      set_data(res, key, value, type);
    } else if (client) {
      _write(client, "$-1\r\n", 5);
    }
  } else {
    set_data(res, key, value, type);
    if (client) _write(client, "+OK\r\n", 5);
  }
}

struct Command cmd_set = {
  .name = "SET",
  .summary = "Sets value to specified key. If the key already has a value, it is overwritten.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
