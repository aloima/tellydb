#include "../../headers/telly.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>

static struct Configuration *conf;

void initialize_logs(struct Configuration *config) {
  conf = config;
}

void write_log(enum LogLevel level, const char *fmt, ...) {
  const uint8_t check = conf->allowed_log_levels & level;
  time_t raw;
  time(&raw);

  const uint32_t buf_size = conf->max_log_len + 1;
  char buf[buf_size];
  va_list args;

  va_start(args, fmt);
  vsnprintf(buf, buf_size, fmt, args);
  va_end(args);

  switch (check) {
    case LOG_INFO:
      fprintf(stdout, "[%.24s / INFO]: %s\n", ctime(&raw), buf);
      break;

    case LOG_WARN:
      fprintf(stdout, "[%.24s / WARN]: %s\n", ctime(&raw), buf);
      break;

    case LOG_ERR:
      fprintf(stderr, "[%.24s / ERR]: %s\n", ctime(&raw), buf);
      break;
  }
}
