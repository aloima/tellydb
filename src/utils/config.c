#include "../../headers/telly.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static struct Configuration default_conf = {
  .port = 6379,
  .max_clients = 128,
  .allowed_log_levels = LOG_INFO | LOG_ERR | LOG_WARN,
  .max_log_len = 8192,
  .data_file = ".tellydb",
  .log_file = ".tellylog"
};

static void pass_line(FILE *file, char c) {
  while (c != EOF && c != '\n') c = fgetc(file);
}

static void parse_value(FILE *file, char *buf) {
  char c = 1;

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
          memset(buf, 0, 49);
          parse_value(file, buf);
          conf.port = atoi(buf);
        } else if (streq(buf, "MAX_CLIENTS")) {
          memset(buf, 0, 49);
          parse_value(file, buf);
          conf.max_clients = atoi(buf);
        } else if (streq(buf, "ALLOWED_LOG_LEVELS")) {
          memset(buf, 0, 49);
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
        } else if (streq(buf, "MAX_LOG_LEN")) {
          memset(buf, 0, 49);
          parse_value(file, buf);
          conf.max_log_len = atoi(buf);
        } else if (streq(buf, "DATA_FILE")) {
          memset(conf.data_file, 0, 49);
          parse_value(file, conf.data_file);
        } else if (streq(buf, "LOG_FILE")) {
          memset(conf.log_file, 0, 49);
          parse_value(file, conf.log_file);
        } else {
          return conf;
        }

        memset(buf, 0, 49);
        break;

      default:
        strncat(buf, &c, 1);
    }
  } while (c != EOF);

  return conf;
}

static void get_allowed_log_levels(char *allowed_log_levels, struct Configuration conf) {
  enum LogLevel log_levels[3] = {LOG_ERR, LOG_INFO, LOG_WARN};
  uint32_t len = 0;

  for (uint32_t i = 0; i < 3; ++i) {
    enum LogLevel level = log_levels[i];

    if (conf.allowed_log_levels & level) {
      switch (level) {
        case LOG_ERR:
          allowed_log_levels[len] = 'e';
          len += 1;
          break;

        case LOG_WARN:
          allowed_log_levels[len] = 'w';
          len += 1;
          break;

        case LOG_INFO:
          allowed_log_levels[len] = 'i';
          len += 1;
          break;
      }
    }
  }

  allowed_log_levels[len] = 0;
}

uint32_t get_configuration_string(char *buf, struct Configuration conf) {
  char allowed_log_levels[4];
  get_allowed_log_levels(allowed_log_levels, conf);

  return sprintf(buf, (
    "# TCP server port\n"
    "PORT=%d\n\n"
    "# Specifies max connactable client count, higher values may cause higher resource usage\n"
    "MAX_CLIENTS=%d\n\n"
    "# Allowed log levels:\n"
    "# w = warning\n"
    "# i = information\n"
    "# e = error\n\n"
    "# Order of keys does not matter\n"
    "ALLOWED_LOG_LEVELS=%s\n\n"
    "# Specifies maximum writeable log length to STDOUT\n"
    "MAX_LOG_LEN=%d\n\n"
    "# Specifies database file where data will be saved\n"
    "DATA_FILE=%s\n\n"
    "# Specifies log file where logs will be appended\n"
    "LOG_FILE=%s\n"
  ), conf.port, conf.max_clients, allowed_log_levels, conf.max_log_len, conf.data_file, conf.log_file);
}

struct Configuration get_default_configuration() {
  return default_conf;
}

struct Configuration *get_configuration(const char *filename) {
  struct Configuration *conf = malloc(sizeof(struct Configuration));

  if (filename == NULL) {
    FILE *file = fopen(".tellyconf", "r");

    if (file != NULL) {
      struct Configuration data = parse_configuration(file);
      memcpy(conf, &data, sizeof(struct Configuration));
      fclose(file);
    } else {
      memcpy(conf, &default_conf, sizeof(struct Configuration));
    }

    return conf;
  } else {
    FILE *file = fopen(filename, "r");

    if (file != NULL) {
      struct Configuration data = parse_configuration(file);
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
