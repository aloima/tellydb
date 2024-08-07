#include "../telly.h"

#include <stdio.h>
#include <stdlib.h>

static struct Configuration default_conf = {
  .port = 6379
};

void parse_value(FILE *file, char *buf) {
  char c;

  do {
    c = fgetc(file);
    strncat(buf, &c, 1);
  } while (c != EOF && c != '\n');
}

struct Configuration parse_configuration(FILE *file) {
  struct Configuration conf = {0};
  char buf[64] = {0};
  char c;

  do {
    c = fgetc(file);

    if (c == '=') {
      if (streq(buf, "PORT")) {
        memset(buf, 0, 64);
        parse_value(file, buf);
        conf.port = atoi(buf);
      } else {
        return conf;
      }

      memset(buf, 0, 64);
    }

    strncat(buf, &c, 1);
  } while (c != EOF);

  return conf;
}

void get_configuration_string(char *buf, struct Configuration conf) {
  sprintf(buf, (
    "PORT=%d\n"
  ), conf.port);
}

struct Configuration get_default_configuration() {
  return default_conf;
}

struct Configuration get_configuration(const char *filename) {
  if (filename == NULL) {
    FILE *file = fopen(".tellyconf", "r");

    if (file != NULL) {
      struct Configuration conf = parse_configuration(file);
      fclose(file);

      return conf;
    } else {
      return default_conf;
    }
  } else {
    FILE *file = fopen(filename, "r");

    if (file != NULL) {
      struct Configuration conf = parse_configuration(file);
      fclose(file);

      return conf;
    } else {
      return get_configuration(NULL);
    }
  }
}
