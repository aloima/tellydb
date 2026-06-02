#include <telly.h>

static void get_keys(struct CommandEntry *entry) {
  if (entry->args->count < 2) return;

  (void) insert_into_vector(server->keyspace, &entry->args->data[0]);
}


typedef struct Response {
  const char *input;
  enum TellyTypes type;

  bool is_integer, is_double;
  bool is_true;
} Response;

typedef struct Options {
  bool get;
  bool nx, xx;
  bool as;

  Expiry expiry;
} Options;

typedef enum OptionParsingCode : uint8_t {
  MISSING_EX_VALUE,
  INVALID_EX_VALUE,
  SYSCALL_ERROR_FOR_EX,

  MISSING_PX_VALUE,
  INVALID_PX_VALUE,
  SYSCALL_ERROR_FOR_PX,

  INVALID_TYPE_IN_AS,
  INVALID_OPTIONS,

  MUST_BE_NUMBER,
  MUST_BE_BOOLEAN,
  MUST_BE_DOUBLE,
  MUST_BE_INTEGER,

  SIMULTANEOUSLY_NX_XX,

  VALID_OPTIONS
} OptionParsingCode;

static constexpr string_t parsing_code_response[] = {
  [MISSING_EX_VALUE]     = RESP_ERROR_MESSAGE("There is no specified value for 'EX' argument"),
  [INVALID_EX_VALUE]     = RESP_ERROR_MESSAGE("The value of 'EX' argument must be an integer"),
  [SYSCALL_ERROR_FOR_EX] = RESP_ERROR_MESSAGE("Cannot take the current time for 'EX' argument"),

  [MISSING_PX_VALUE]     = RESP_ERROR_MESSAGE("There is no specified value for 'PX' argument"),
  [INVALID_PX_VALUE]     = RESP_ERROR_MESSAGE("The value of 'PX' argument must be an integer"),
  [SYSCALL_ERROR_FOR_PX] = RESP_ERROR_MESSAGE("Cannot take the current time for 'PX' argument"),

  [INVALID_TYPE_IN_AS]   = RESP_ERROR_MESSAGE("'AS' argument must be followed by a valid type name for 'SET' command"),
  [INVALID_OPTIONS]      = RESP_ERROR_MESSAGE("Invalid argument(s) for 'SET' command"),

  [MUST_BE_NUMBER]       = RESP_ERROR_MESSAGE("The type must be double or integer for this value"),
  [MUST_BE_BOOLEAN]      = RESP_ERROR_MESSAGE("The type must be boolean for this value"),
  [MUST_BE_DOUBLE]       = RESP_ERROR_MESSAGE("The type must be double for this value"),
  [MUST_BE_INTEGER]      = RESP_ERROR_MESSAGE("The type must be integer for this value"),

  [SIMULTANEOUSLY_NX_XX] = RESP_ERROR_MESSAGE("XX and NX arguments cannot be specified simultaneously for 'SET' command"),

  [VALID_OPTIONS]        = RESP_OK()
};

typedef enum ExpiryType : uint8_t {
  EXPIRY_EX,
  EXPIRY_PX
} ExpiryType;

typedef struct TypeIdentifier {
  const enum TellyTypes type;
  const char **values;
} TypeIdentifier;

static inline OptionParsingCode parse_expiry_option(const ExpiryType type, Options *options, const char *input) {
  static constexpr const OptionParsingCode errors[][2] = {
    [EXPIRY_EX] = {INVALID_EX_VALUE, SYSCALL_ERROR_FOR_EX},
    [EXPIRY_PX] = {INVALID_PX_VALUE, SYSCALL_ERROR_FOR_PX}
  };

  if (!try_parse_integer(input))
    return errors[type][0]; // INVALID_VALUE

  struct timespec expire;
  if (clock_gettime(CLOCK_REALTIME, &expire) == -1)
    return errors[type][1]; // SYSCALL_ERROR

  const uint64_t value = strtoull(input, (char **) NULL, 10);
  options->expiry.at = (expire.tv_sec * 1000) + (expire.tv_nsec / 1e6);

  switch (type) {
    case EXPIRY_EX:
      options->expiry.at += (value * 1000);
      break;

    case EXPIRY_PX:
      options->expiry.at += value;
      break;
  }

  unreachable();
}

static inline void take_as_number(Response *response) {
  response->is_integer = try_parse_integer(response->input);
  response->is_double = try_parse_double(response->input);

  if (response->is_integer) {
    response->type = TELLY_INT;
    return;
  } else if (response->is_double) {
    response->type = TELLY_DOUBLE;
    return;
  }

  response->type = TELLY_UNKNOWN;
}

static OptionParsingCode parse_options(struct CommandEntry *entry, Options *options, Response *response) {
  const TypeIdentifier types[] = {
    {TELLY_STR,     (const char *[]) {"STR", "STRING", NULL}},
    {TELLY_BOOL,    (const char *[]) {"BOOL", "BOOLEAN", NULL}},
    {TELLY_NULL,    (const char *[]) {"NULL", NULL}},
    {TELLY_INT,     (const char *[]) {"INT", "INTEGER", NULL}},
    {TELLY_DOUBLE,  (const char *[]) {"DOUBLE", NULL}},
    {TELLY_UNKNOWN, (const char *[]) {"NUM", "NUMBER", NULL}},
  };

  for (uint32_t i = 2; i < entry->args->count; ++i) {
    const string_t arg = entry->args->data[i];
    to_uppercase(arg, arg.value);

    if (streq(arg.value, "GET")) {
      options->get = true;
    } else if (streq(arg.value, "NX")) {
      if (options->xx)
        return SIMULTANEOUSLY_NX_XX;

      options->nx = true;
    } else if (streq(arg.value, "XX")) {
      if (options->nx)
        return SIMULTANEOUSLY_NX_XX;

      options->xx = true;
    } else if (streq(arg.value, "EX")) {
      if ((i + 1) >= entry->args->count)
        return MISSING_EX_VALUE;

      options->expiry.enabled = true;
      i += 1;

      parse_expiry_option(EXPIRY_EX, options, entry->args->data[i].value);
    } else if (streq(arg.value, "PX")) {
      if ((i + 1) >= entry->args->count)
        return MISSING_PX_VALUE;

      options->expiry.enabled = true;
      i += 1;

      parse_expiry_option(EXPIRY_EX, options, entry->args->data[i].value);
    } else if (streq(arg.value, "AS")) {
      options->as = true;

      if ((i + 1) < entry->args->count) {
        i += 1;
        string_t name = entry->args->data[i];
        to_uppercase(name, name.value);

        for (uint32_t j = 0; j < sizeof(types) / sizeof(TypeIdentifier); ++j) {
          const TypeIdentifier identifier = types[j];
          uint32_t k = 0;

          while (identifier.values[k] != NULL) {
            if (streq(name.value, identifier.values[k])) {
              response->type = identifier.type;
              j = UINT32_MAX - 1; // `goto` is unpredictable action for compiler
              break;
            }

            k += 1;
          }
        }

        switch (response->type) {
          case TELLY_UNKNOWN:
            if (streq(name.value, "NUM") || streq(name.value, "NUMBER")) {
              take_as_number(response);
              if (response->type == TELLY_UNKNOWN)
                return MUST_BE_NUMBER;

              break;
            }

            return INVALID_TYPE_IN_AS;

          case TELLY_INT:
            response->is_integer = try_parse_integer(response->input);
            if (!response->is_integer)
              return MUST_BE_INTEGER;

            break;

          case TELLY_DOUBLE:
            response->is_double = try_parse_double(response->input);
            if (!response->is_double)
              return MUST_BE_DOUBLE;

            break;

          case TELLY_BOOL:
            if (!response->is_true && !streq(response->input, "false"))
              return MUST_BE_BOOLEAN;

            break;

          default:
            break;
        }

        return VALID_OPTIONS;
      }
    }

    return INVALID_OPTIONS;
  }

  return VALID_OPTIONS;
}

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

  const char *input = entry->args->data[1].value;

  Response response = {
    .input = input,
    .type = TELLY_UNKNOWN,

    .is_true = streq(input, "true"),
    .is_integer = false,
    .is_double = false
  };

  Options options = {
    .as = false,
    .expiry.enabled = false,
    .get = false,
    .nx = false,
    .xx = false
  };

  const OptionParsingCode options_code = parse_options(entry, &options, &response);

  if (options_code != VALID_OPTIONS) {
    PASS_NO_CLIENT(entry->client);
    const string_t error = parsing_code_response[options_code];
    ASSERT(error.len, !=, 0U);

    return error;
  }

  const string_t key = entry->args->data[0];
  void *value;
  KeyValue *res = get_data(entry->database, key);

  if (options.nx && res) {
    PASS_NO_CLIENT(entry->client);
    return RESP_NULL(entry->client->protover);
  }

  if (options.xx && !res) {
    PASS_NO_CLIENT(entry->client);
    return RESP_NULL(entry->client->protover);
  }

  if (!options.as) {
    response.is_integer = try_parse_integer(response.input);
    response.is_double = try_parse_double(response.input);
  }

  if (response.is_integer || response.is_double) {
    if (!options.as)
      response.type = (response.is_integer ? TELLY_INT : TELLY_DOUBLE);

    switch (response.type) {
      case TELLY_INT:
        value = malloc(sizeof(mpz_t));
        if (value == NULL) {
          PASS_NO_CLIENT(entry->client);
          return OUT_OF_MEMORY();
        }

        mpz_init_set_str(*((mpz_t *) value), response.input, 10);
        break;

      case TELLY_DOUBLE:
        value = malloc(sizeof(mpf_t));
        if (value == NULL) {
          PASS_NO_CLIENT(entry->client);
          return OUT_OF_MEMORY();
        }

        mpf_init2(*((mpf_t *) value), FLOAT_PRECISION);
        mpf_set_str(*((mpf_t *) value), response.input, 10);
        break;

      case TELLY_STR:
        if (take_as_string(&value, entry->args->data[1]) == -1) {
          PASS_NO_CLIENT(entry->client);
          return OUT_OF_MEMORY();
        }

        break;

      default:
        break;
    }
  } else if (response.is_true || streq(response.input, "false")) {
    if (!options.as)
      response.type = TELLY_BOOL;

    switch (response.type) {
      case TELLY_BOOL:
        value = malloc(sizeof(bool));
        if (value == NULL) {
          PASS_NO_CLIENT(entry->client);
          return OUT_OF_MEMORY();
        }

        *((bool *) value) = response.is_true;
        break;

      case TELLY_STR:
        if (take_as_string(&value, entry->args->data[1]) == -1) {
          PASS_NO_CLIENT(entry->client);
          return OUT_OF_MEMORY();
        }

        break;

      default:
        break;
    }
  } else if (streq(response.input, "null")) {
    if (!options.as)
      response.type = TELLY_NULL;

    switch (response.type) {
      case TELLY_NULL:
        value = NULL;
        break;

      case TELLY_STR:
        if (take_as_string(&value, entry->args->data[1]) == -1) {
          PASS_NO_CLIENT(entry->client);
          return OUT_OF_MEMORY();
        }

        break;

      default:
        break;
    }
  } else {
    if (!options.as)
      response.type = TELLY_STR;

    if (take_as_string(&value, entry->args->data[1]) == -1) {
      PASS_NO_CLIENT(entry->client);
      return OUT_OF_MEMORY();
    }
  }

  if (options.get) {
    if (entry->password->permissions & P_READ) {
      const uint64_t *expire_at = (options.expiry.enabled ? &options.expiry.at : NULL);
      const bool success = (set_data(entry->database, res, key, value, response.type, expire_at) != NULL);

      if (!success) {
        PASS_NO_CLIENT(entry->client);
        return OUT_OF_MEMORY();
      }

      if (res) {
        if (entry->client) {
          return write_value(value, response.type, entry->client->protover, entry->client->write_buf);
        }
      }

      PASS_NO_CLIENT(entry->client);
      return RESP_NULL(entry->client->protover);
    } else {
      PASS_NO_CLIENT(entry->client);
      return RESP_ERROR_MESSAGE("Not allowed to use this command, need P_READ");
    }
  } else {
    const uint64_t *expire_at = (options.expiry.enabled ? &options.expiry.at : NULL);
    const bool success = (set_data(entry->database, res, key, value, response.type, expire_at) != NULL);
    PASS_NO_CLIENT(entry->client);

    return (success ? RESP_OK() : OUT_OF_MEMORY());
  }
}

const struct Command cmd_set = {
  .name = "SET",
  .summary = "Set key to hold the string value.",
  .since = "0.1.0",
  .complexity = "O(1)",
  .permissions = P_WRITE,
  .flags.value = (CMD_FLAG_ACCESS_DATABASE | CMD_FLAG_AFFECT_DATABASE),
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
