#include <telly.h>

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <gmp.h>

static inline int take_as_string(void **value, const string_t data) {
  string_t *string = (*value = malloc(sizeof(string_t)));
  if (string == NULL) return -1;

  string->len = data.len;
  string->value = malloc(string->len);

  if (string->value == NULL) {
    free(string);
    return -1;
  }

  memcpy(string->value, data.value, string->len);
  return 0;
}

static string_t run(struct CommandEntry *entry) {
  if (entry->args->count < 2) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("SET");
  }

  char *value_in = entry->args->data[1].value;
  const bool is_true = streq(value_in, "true");
  bool is_integer = false;
  bool is_double = false;

  bool get = false;
  bool nx = false, xx = false, as = false;

  enum TellyTypes type;

  for (uint32_t i = 2; i < entry->args->count; ++i) {
    string_t arg = entry->args->data[i];
    to_uppercase(arg, arg.value);

    if (streq(arg.value, "GET")) {
      get = true;
    } else if (streq(arg.value, "NX")) {
      nx = true;
    } else if (streq(arg.value, "XX")) {
      xx = true;
    } else if (streq(arg.value, "AS")) {
      as = true;

      if ((i + 1) < entry->args->count) {
        i += 1;
        string_t name = entry->args->data[i];
        to_uppercase(name, name.value);

        if (streq(name.value, "STRING") || streq(name.value, "STR")) {
          type = TELLY_STR;
        } else if (streq(name.value, "BOOLEAN") || streq(name.value, "BOOL")) {
          type = TELLY_BOOL;
        } else if (streq(name.value, "INTEGER") || streq(name.value, "INT")) {
          is_integer = try_parse_integer(value_in);
          type = TELLY_INT;
        } else if (streq(name.value, "NUMBER") || streq(name.value, "NUM")) {
          is_integer = try_parse_integer(value_in);
          is_double = try_parse_double(value_in);

          if (is_integer) {
            type = TELLY_INT;
          } else if (is_double) {
            type = TELLY_DOUBLE;
          }
        } else if (streq(name.value, "DOUBLE")) {
          is_double = try_parse_double(value_in);
          type = TELLY_DOUBLE;
        } else if (streq(name.value, "NULL")) {
          type = TELLY_NULL;
        } else {
          PASS_NO_CLIENT(entry->client);
          return RESP_ERROR_MESSAGE("'AS' argument must be followed by a valid type name for 'SET' command");;
        }
      }
    } else {
      PASS_NO_CLIENT(entry->client);
      return RESP_ERROR_MESSAGE("Invalid argument(s) for 'SET' command");;
    }
  }

  if (nx && xx) {
    PASS_NO_CLIENT(entry->client);
    return RESP_ERROR_MESSAGE("XX and NX arguments cannot be specified simultaneously for 'SET' command");
  }

  const string_t key = entry->args->data[0];
  void *value;
  struct KVPair *res = get_data(entry->database, key);

  if (nx && res) {
    PASS_NO_CLIENT(entry->client);
    return RESP_NULL(entry->client->protover);
  }

  if (xx && !res) {
    PASS_NO_CLIENT(entry->client);
    return RESP_NULL(entry->client->protover);
  }

  if (!as) {
    is_integer = try_parse_integer(value_in);
    is_double = try_parse_double(value_in);
  }

  if (is_integer || is_double) {
    if (!as) {
      type = (is_integer ? TELLY_INT : TELLY_DOUBLE);
    } else if (type != TELLY_INT && type != TELLY_DOUBLE && type != TELLY_STR) {
      PASS_NO_CLIENT(entry->client);
      return RESP_ERROR_MESSAGE("The type must be string or integer for this value");;
    }

    switch (type) {
      case TELLY_INT: {
        value = malloc(sizeof(mpz_t));
        if (value == NULL) return RESP_ERROR_MESSAGE("Out of memory");

        mpz_init_set_str(*((mpz_t *) value), value_in, 10);
        break;
      }

      case TELLY_DOUBLE: {
        value = malloc(sizeof(mpf_t));
        if (value == NULL) return RESP_ERROR_MESSAGE("Out of memory");

        mpf_init2(*((mpf_t *) value), FLOAT_PRECISION);
        mpf_set_str(*((mpf_t *) value), value_in, 10);
        break;
      }

      case TELLY_STR: {
        take_as_string(&value, entry->args->data[1]);
        break;
      }

      default:
        break;
    }
  } else if (is_true || streq(value_in, "false")) {
    if (!as) {
      type = TELLY_BOOL;
    } else if (type != TELLY_BOOL && type != TELLY_STR) {
      PASS_NO_CLIENT(entry->client);
      return RESP_ERROR_MESSAGE("The type must be string or boolean for this value");;
    }

    switch (type) {
      case TELLY_BOOL:
        value = malloc(sizeof(bool));
        if (value == NULL) return RESP_ERROR_MESSAGE("Out of memory");

        *((bool *) value) = is_true;
        break;

      case TELLY_STR:
        if (take_as_string(&value, entry->args->data[1]) == -1) {
          PASS_NO_CLIENT(entry->client);
          return RESP_ERROR_MESSAGE("Out of memory");
        }

        break;

      default:
        break;
    }
  } else if (streq(value_in, "null")) {
    if (!as) {
      type = TELLY_NULL;
    } else if (type != TELLY_NULL && type != TELLY_STR) {
      PASS_NO_CLIENT(entry->client);
      return RESP_ERROR_MESSAGE("The type must be string or null for this value");
    }

    switch (type) {
      case TELLY_NULL:
        value = NULL;
        break;

      case TELLY_STR:
        if (take_as_string(&value, entry->args->data[1]) == -1) {
          PASS_NO_CLIENT(entry->client);
          return RESP_ERROR_MESSAGE("Out of memory");
        }

        break;

      default:
        break;
    }
  } else {
    if (!as) {
      type = TELLY_STR;
    } else if (type != TELLY_STR) {
      PASS_NO_CLIENT(entry->client);
      return RESP_ERROR_MESSAGE("The type must be string for this value");
    }

    if (take_as_string(&value, entry->args->data[1]) == -1) {
      PASS_NO_CLIENT(entry->client);
      return RESP_ERROR_MESSAGE("Out of memory");
    }
  }

  if (get) {
    if (entry->password->permissions & P_READ) {
      const bool success = (set_data(entry->database, res, key, value, type) != NULL);

      if (!success) {
        PASS_NO_CLIENT(entry->client);
        return RESP_ERROR_MESSAGE("Out of memory");
      }

      if (res) {
        if (entry->client) {
          return write_value(value, type, entry->client->protover, entry->client->write_buf);
        }
      }

      PASS_NO_CLIENT(entry->client);
      return RESP_NULL(entry->client->protover);
    } else {
      PASS_NO_CLIENT(entry->client);
      return RESP_ERROR_MESSAGE("Not allowed to use this command, need P_READ");
    }
  } else {
    const bool success = (set_data(entry->database, res, key, value, type) != NULL);
    PASS_NO_CLIENT(entry->client);

    if (success) {
      return RESP_OK();
    } else {
      return RESP_ERROR_MESSAGE("Out of memory");
    }
  }
}

const struct Command cmd_set = {
  .name = "SET",
  .summary = "Sets value.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .permissions = P_WRITE,
  .flags = CMD_FLAG_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
