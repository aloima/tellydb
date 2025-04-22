#include "../../../headers/telly.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

static void take_as_string(void **value, const string_t data) {
  string_t *string = (*value = malloc(sizeof(string_t)));
  string->len = data.len;
  string->value = malloc(string->len);
  memcpy(string->value, data.value, string->len);
}

static void run(struct CommandEntry entry) {
  if (entry.data->arg_count < 2) {
    if (entry.client) WRONG_ARGUMENT_ERROR(entry.client, "SET", 3);
    return;
  }

  char *value_in = entry.data->args[1].value;
  bool get = false;
  bool nx = false, xx = false, as = false;

  enum TellyTypes type;

  for (uint32_t i = 2; i < entry.data->arg_count; ++i) {
    string_t input = entry.data->args[i];
    char *arg = malloc(input.len + 1);
    to_uppercase(input.value, arg);

    if (streq(arg, "GET")) get = true;
    else if (streq(arg, "NX")) nx = true;
    else if (streq(arg, "XX")) xx = true;
    else if (streq(arg, "AS")) {
      as = true;

      if ((i + 1) < entry.data->arg_count) {
        i += 1;
        input = entry.data->args[i];

        char *type_name = malloc(input.len + 1);
        to_uppercase(input.value, type_name);

        if (streq(type_name, "STRING") || streq(type_name, "STR")) type = TELLY_STR;
        else if (streq(type_name, "BOOLEAN") || streq(type_name, "BOOL")) type = TELLY_BOOL;
        else if (streq(type_name, "NUMBER") || streq(type_name, "NUM") || streq(type_name, "INTEGER") || streq(type_name, "INT")) type = TELLY_NUM;
        else if (streq(type_name, "NULL")) type = TELLY_NULL;
        else {
          if (entry.client) _write(entry.client, "-'AS' argument must be followed by a valid type name for 'SET' command\r\n", 72);
          free(arg);
          free(type_name);
          return;
        }

        free(type_name);
      }
    } else {
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
    if (!as) type = TELLY_NUM;
    else if (type != TELLY_NUM && type != TELLY_STR) {
      if (entry.client) _write(entry.client, "-The type must be string or integer for this value\r\n", 52);
      return;
    }

    switch (type) {
      case TELLY_NUM: {
        const long number = atol(value_in);
        value = malloc(sizeof(long));
        *((long *) value) = number;
        break;
      }

      case TELLY_STR: {
        take_as_string(&value, entry.data->args[1]);
        break;
      }

      default:
        break;
    }
  } else if (is_true || streq(value_in, "false")) {
    if (!as) type = TELLY_BOOL;
    else if (type != TELLY_BOOL && type != TELLY_STR) {
      if (entry.client) _write(entry.client, "-The type must be string or boolean for this value\r\n", 52);
      return;
    }

    switch (type) {
      case TELLY_BOOL: {
        value = malloc(sizeof(bool));
        *((bool *) value) = is_true;
        break;
      }

      case TELLY_STR: {
        take_as_string(&value, entry.data->args[1]);
        break;
      }

      default:
        break;
    }
  } else if (streq(value_in, "null")) {
    if (!as) type = TELLY_NULL;
    else if (type != TELLY_NULL && type != TELLY_STR) {
      if (entry.client) _write(entry.client, "-The type must be string or null for this value\r\n", 49);
      return;
    }

    switch (type) {
      case TELLY_NULL: {
        value = NULL;
        break;
      }

      case TELLY_STR: {
        take_as_string(&value, entry.data->args[1]);
        break;
      }

      default:
        break;
    }
  } else {
    if (!as) type = TELLY_STR;
    else if (type != TELLY_STR) {
      if (entry.client) _write(entry.client, "-The type must be string for this value\r\n", 41);
      return;
    }

    take_as_string(&value, entry.data->args[1]);
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
