#include <telly.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

static Config default_conf = {
  .port = 6379,
  .max_clients = 128,
  .max_transaction_blocks = 262144,
  .allowed_log_levels = LOG_INFO | LOG_ERR | LOG_WARN | LOG_DBG,
  .max_log_lines = 128,
  .data_file = ".tellydb",
  .log_file = ".tellylog",
  .database_name = "telly",

  .tls = false,
  .cert = {0},
  .private_key = {0}
};

typedef struct {
  char ident;
  uint8_t value;
} LogLevelField;

typedef struct {
  const char *key;
  void *value;
  bool (*setter)(void *value, const char *buf);
} ConfigEntry;

static constexpr LogLevelField log_levels_map[4] = {
  {'i', LOG_INFO},
  {'e', LOG_ERR},
  {'w', LOG_WARN},
  {'d', LOG_DBG}
};

static inline void pass_line(FILE *file) {
  int c;
  while ((c = fgetc(file)) != EOF && c != '\n')
    ;
}

static inline void read_value(FILE *file, char *buf) {
  int c;

  while ((c = fgetc(file)) != EOF && c != '\n') {
    if (c != ' ') strncat(buf, (const char *) &c, 1);
  }
}

static inline void get_allowed_log_levels(char *allowed_log_levels, const uint8_t value) {
  for (uint32_t i = 0; i < (sizeof(log_levels_map) / sizeof(log_levels_map)[0]); ++i) { // Loop unrolling by compiler
    const LogLevelField level = log_levels_map[i];

    if (value & level.value) {
      *(allowed_log_levels++) = level.ident;
    }
  }

  *allowed_log_levels = '\0';
}

static inline bool set_uint(void *value, const char *buf) {
  if (buf[0] == '-') return false;

  char *end;
  *((uint64_t *) value) = strtoull(buf, &end, 10);

  return (*end == '\0');
}

static inline bool set_int(void *value, const char *buf) {
  char *end;
  *((uint64_t *) value) = strtoll(buf, &end, 10);

  return (*end == '\0');
}

static inline bool set_str(void *value, const char *buf) {
  strcpy(value, buf);
  return true;
}

static inline bool set_bool(void *value, const char *buf) {
  const bool enabled = streq(buf, "true");

  if (enabled || streq(buf, "false")) {
    *((bool *) value) = enabled;
  } else {
    return false;
  }

  return true;
}

static inline bool set_allowed_log_levels(void *value, const char *buf) {
  uint8_t *log_levels = value;
  char id = *buf;

  while (id != '\0') {
    const uint32_t size = (sizeof(log_levels_map) / sizeof(log_levels_map)[0]);
    uint32_t j = 0;

    while (j < size) {
      const LogLevelField level = log_levels_map[j];

      if (level.ident == id) {
        *log_levels |= level.value;
        break;
      }

      j += 1;
    }

    if (j == size) return false;
    id = *(++buf);
  }

  return true;
}

 Config parse_config(FILE *file) {
  Config conf = {0};
  char buf[49] = {0};
  int c;

  ConfigEntry entries[] = {
    {"port", &conf.port, set_uint},
    {"max_clients", &conf.max_clients, set_uint},
    {"max_transaction_blocks", &conf.max_transaction_blocks, set_uint},

    {"allowed_log_levels", &conf.allowed_log_levels, set_allowed_log_levels},
    {"max_log_lines", &conf.max_log_lines, set_int},
    {"log_file", &conf.log_file, set_str},

    {"data_file", &conf.data_file, set_str},
    {"database_name", &conf.database_name, set_str},

    {"tls", &conf.tls, set_bool},
    {"cert", &conf.cert, set_str},
    {"private_key", &conf.private_key, set_str},
  };

  const uint32_t count = (sizeof(entries) / sizeof(entries[0]));

  do {
    c = fgetc(file);

    switch (c) {
      case '#': case '\n':
        pass_line(file);
        break;

      case '=':
        uint32_t i = 0;

        while (i < count) {
          const ConfigEntry *entry = &entries[i];
          if (streq(buf, entry->key)) break;

          i += 1;
        }

        if (i == count) {
          write_log(LOG_ERR, "Cannot parse configuration, invalid key: '%s'", buf);
          exit(EXIT_FAILURE);
        }

        buf[0] = '\0';
        read_value(file, buf);

        const ConfigEntry *entry = &entries[i];
        const bool res = entry->setter(entry->value, buf);
        if (!res) {
          write_log(LOG_ERR, "Cannot parse configuration, invalid value for '%s'.", entry->key);
          exit(EXIT_FAILURE);
        }

        buf[0] = '\0';
        break;

      case ' ':
        break;

      default:
        strncat(buf, (const char *) &c, 1);
    }
  } while (c != EOF);

  if (conf.max_clients > UINT16_MAX) {
    write_log(LOG_ERR, "'MAX_CLIENTS' in configuration violates limitations: value <= %u", UINT16_MAX);
    exit(EXIT_FAILURE);
  } else if (conf.max_transaction_blocks > UINT32_MAX) {
    write_log(LOG_ERR, "'MAX_TRANSACTION_BLOCKS' in configuration violates limitations: value <= %u", UINT32_MAX);
    exit(EXIT_FAILURE);
  } else if (conf.max_log_lines > INT32_MAX || conf.max_log_lines <= 0) {
    write_log(LOG_ERR, "'MAX_TRANSACTION_BLOCKS' in configuration violates limitations: 0 < value <= %d", INT32_MAX);
    exit(EXIT_FAILURE);
  }

  return conf;
}

size_t get_config_string(char *buf, Config *conf) {
  char allowed_log_levels[5];
  get_allowed_log_levels(allowed_log_levels, conf->allowed_log_levels);

  return sprintf(buf, (
    "# TCP server port\n"
    "port = %" PRIu16 "\n\n"

    "# Specifies max connectable client count, higher values may cause higher resource usage\n"
    "max_clients = %" PRIu16 "\n\n"

    "# Specifies max storable transaction block count, higher values may cause higher resource usage\n"
    "# 2^n values are possible. If it is not 2^n, it will be rounded down.\n"
    "max_transaction_blocks = %" PRIu32 "\n\n"

    "# Allowed log levels:\n"
    "# w = warning\n"
    "# i = information\n"
    "# e = error\n"
    "# d = debug\n\n"
    "# Order of keys does not matter\n"
    "allowed_log_levels = %s\n\n"

    "# Specifies maximum line count of logs will be saved to log file, to make undetermined, change it to -1.\n"
    "# If the log file contains more log lines than this value, will not be deleted old logs and will not be saved new logs.\n"
    "# MAX_LOG_LINES * (FILE BLOCK SIZE [512, 4096 or a power of 2] + 1) bytes will be allocated, so be careful\n"
    "max_log_lines = %" PRIi32 "\n\n"

    "# Specifies database file where data will be saved\n"
    "data_file = %s\n\n"

    "# Specifies log file where logs will be saved\n"
    "log_file = %s\n\n"

    "# Specifies default database name, it will be created on first startup of server and deletion of all databases\n"
    "# On a client connection to the server, the client will be paired with this database\n"
    "# This length must be less than or equal to 64\n"
    "database_name = %s\n\n"

    "# Enables/disables creating TLS server\n"
    "#\n"
    "# If it is enabled:\n"
    "# CERT specifies certificate file path of TLS server\n"
    "# PRIVATE_KEY specifies private key file path of TLS server\n"
    "# TLS value must be true or false\n"
    "# File paths length must be less than or equal to 48\n"
    "tls = %s\n"
    "cert = %s\n"
    "private_key = %s\n"
  ), conf->port, conf->max_clients, conf->max_transaction_blocks, allowed_log_levels, conf->max_log_lines, conf->data_file,
     conf->log_file, conf->database_name, conf->tls ? "true" : "false", conf->cert, conf->private_key
  );
}

Config *get_default_config() {
  return &default_conf;
}

Config *get_config(const char *filename) {
  Config *conf = malloc(sizeof(Config));
  if (!conf) {
    write_log(LOG_ERR, "Allocation failed for get_config");
    return NULL;
  }

  if (filename == NULL) {
    FILE *file = fopen(".tellyconf", "r");

    if (file) {
      const Config data = parse_config(file);
      memcpy(conf, &data, sizeof(Config));
      fclose(file);
    } else {
      memcpy(conf, &default_conf, sizeof(Config));
    }

    return conf;
  } else {
    FILE *file = fopen(filename, "r");

    if (file) {
      const Config data = parse_config(file);
      memcpy(conf, &data, sizeof(Config));
      fclose(file);

      return conf;
    } else {
      free(conf);
      return get_config(NULL);
    }
  }
}

void free_config(Config *conf) {
  if (conf != &default_conf) free(conf);
}
