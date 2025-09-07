#include <telly.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

static struct Configuration default_conf = {
  .port = 6379,
  .max_clients = 128,
  .max_transaction_blocks = 262144,
  .allowed_log_levels = LOG_INFO | LOG_ERR | LOG_WARN,
  .max_log_lines = 128,
  .data_file = ".tellydb",
  .log_file = ".tellylog",
  .database_name = "telly",

  .tls = false,
  .cert = {0},
  .private_key = {0},

  .default_conf = true
};

static void pass_line(FILE *file, char c) {
  while (c != EOF && c != '\n') c = fgetc(file);
}

static void parse_value(FILE *file, char *buf) {
  char c;

  while ((c = fgetc(file)) != EOF && c != '\n') {
    strncat(buf, &c, 1);
  }
}

struct Configuration parse_configuration(FILE *file) {
  struct Configuration conf = {0};
  char buf[49] = {0};
  char c;

  do {
    c = fgetc(file);

    switch (c) {
      case '#':
        pass_line(file, c);
        break;

      case '\n':
        pass_line(file, c);
        break;

      case '=':
        if (streq(buf, "PORT")) {
          buf[0] = '\0';
          parse_value(file, buf);
          conf.port = atoi(buf);
        } else if (streq(buf, "MAX_CLIENTS")) {
          buf[0] = '\0';
          parse_value(file, buf);
          conf.max_clients = atoi(buf);
        } else if (streq(buf, "MAX_TRANSACTION_BLOCKS")) {
          buf[0] = '\0';
          parse_value(file, buf);
          conf.max_transaction_blocks = atoi(buf);
        } else if (streq(buf, "ALLOWED_LOG_LEVELS")) {
          buf[0] = '\0';
          parse_value(file, buf);

          const uint32_t len = strlen(buf);

          for (uint32_t i = 0; i < len; ++i) {
            switch (buf[i]) {
              case 'i':
                conf.allowed_log_levels |= LOG_INFO;
                break;

              case 'e':
                conf.allowed_log_levels |= LOG_ERR;
                break;

              case 'w':
                conf.allowed_log_levels |= LOG_WARN;
                break;
            }
          }
        } else if (streq(buf, "MAX_LOG_LINES")) {
          buf[0] = '\0';
          parse_value(file, buf);
          conf.max_log_lines = atoi(buf);
        } else if (streq(buf, "DATA_FILE")) {
          parse_value(file, conf.data_file);
        } else if (streq(buf, "LOG_FILE")) {
          parse_value(file, conf.log_file);
        } else if (streq(buf, "DATABASE_NAME")) {
          parse_value(file, conf.database_name);
        } else if (streq(buf, "TLS")) {
          buf[0] = '\0';
          parse_value(file, buf);

          const bool enabled = streq(buf, "true");

          if (enabled || streq(buf, "false")) {
            conf.tls = enabled;
          } else {
            write_log(LOG_ERR, "Cannot parse TLS value of configuration, %s is not a valid value.");
            return conf;
          }
        } else if (streq(buf, "CERT")) {
          parse_value(file, conf.cert);
        } else if (streq(buf, "PRIVATE_KEY")) {
          parse_value(file, conf.private_key);
        } else {
          return conf;
        }

        buf[0] = '\0';
        break;

      default:
        strncat(buf, &c, 1);
    }
  } while (c != EOF);

  return conf;
}

static void get_allowed_log_levels(char *allowed_log_levels, struct Configuration conf) {
  uint32_t len = 0;

  if (conf.allowed_log_levels & LOG_ERR) {
    allowed_log_levels[len] = 'e';
    len += 1;
  }

  if (conf.allowed_log_levels & LOG_WARN) {
    allowed_log_levels[len] = 'w';
    len += 1;
  }

  if (conf.allowed_log_levels & LOG_INFO) {
    allowed_log_levels[len] = 'i';
    len += 1;
  }

  allowed_log_levels[len] = '\0';
}

size_t get_configuration_string(char *buf, struct Configuration conf) {
  char allowed_log_levels[4];
  get_allowed_log_levels(allowed_log_levels, conf);

  return sprintf(buf, (
    "# TCP server port\n"
    "PORT=%" PRIu16 "\n\n"

    "# Specifies max connectable client count, higher values may cause higher resource usage\n"
    "MAX_CLIENTS=%" PRIu16 "\n\n"

    "# Specifies max storable transaction block count, higher values may cause higher resource usage\n"
    "MAX_TRANSACTION_BLOCKS=%" PRIu32 "\n\n"

    "# Allowed log levels:\n"
    "# w = warning\n"
    "# i = information\n"
    "# e = error\n\n"
    "# Order of keys does not matter\n"
    "ALLOWED_LOG_LEVELS=%s\n\n"

    "# Specifies maximum line count of logs will be saved to log file, to make undetermined, change it to -1.\n"
    "# If the log file contains more log lines than this value, will not be deleted old logs and will not be saved new logs.\n"
    "# MAX_LOG_LINES * (FILE BLOCK SIZE [512, 4096 or a power of 2] + 1) bytes will be allocated, so be careful\n"
    "MAX_LOG_LINES=%" PRIi32 "\n\n"

    "# Specifies database file where data will be saved\n"
    "DATA_FILE=%s\n\n"

    "# Specifies log file where logs will be saved\n"
    "LOG_FILE=%s\n\n"

    "# Specifies default database name, it will be created on first startup of server and deletion of all databases\n"
    "# On a client connection to the server, the client will be paired with this database\n"
    "# This length must be less than or equal to 64\n"
    "DATABASE_NAME=%s\n\n"

    "# Enables/disables creating TLS server\n"
    "# If it is enabled, CERT specifies certificate file path of TLS server and PRIVATE_KEY specifies private key file path of TLS server\n"
    "# TLS value must be true or false\n"
    "# File paths length must be less than or equal to 48\n"
    "TLS=%s\n"
    "CERT=%s\n"
    "PRIVATE_KEY=%s\n"
  ), conf.port, conf.max_clients, conf.max_transaction_blocks, allowed_log_levels, conf.max_log_lines, conf.data_file, conf.log_file, conf.database_name,
     conf.tls ? "true" : "false", conf.cert, conf.private_key
  );
}

struct Configuration get_default_configuration() {
  return default_conf;
}

struct Configuration *get_configuration(const char *filename) {
  struct Configuration *conf = malloc(sizeof(struct Configuration));

  if (filename == NULL) {
    FILE *file = fopen(".tellyconf", "r");

    if (file) {
      const struct Configuration data = parse_configuration(file);
      memcpy(conf, &data, sizeof(struct Configuration));
      fclose(file);
    } else {
      memcpy(conf, &default_conf, sizeof(struct Configuration));
    }

    return conf;
  } else {
    FILE *file = fopen(filename, "r");

    if (file) {
      const struct Configuration data = parse_configuration(file);
      memcpy(conf, &data, sizeof(struct Configuration));
      fclose(file);

      return conf;
    } else {
      return get_configuration(NULL);
    }
  }
}

void free_configuration(struct Configuration *conf) {
  free(conf);
}
