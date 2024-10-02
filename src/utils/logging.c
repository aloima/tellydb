#include "../../headers/telly.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>

#include <fcntl.h>
#include <unistd.h>

static struct Configuration *conf;
static int fd = -1;

void initialize_logs(struct Configuration *config) {
  conf = config;

  #if defined(__linux__)
    fd = open(conf->log_file, O_WRONLY | O_CREAT | O_APPEND | O_DIRECT, S_IRWXU);
  #elif defined(__APPLE__)
    fd = open(conf->log_file, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);

    if (fcntl(fd, F_NOCACHE, 1) == -1) {
      write_log(LOG_ERR, "Cannot deactive file caching for database file.");
    }
  #endif
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

  uint32_t message_len = conf->max_log_len + 36;
  char message[message_len + 1];

  switch (check) {
    case LOG_INFO:
      message_len = sprintf(message, "[%.24s / INFO]: %s\n", ctime(&raw), buf);
      break;

    case LOG_WARN:
      message_len = sprintf(message, "[%.24s / WARN]: %s\n", ctime(&raw), buf);
      break;

    case LOG_ERR:
      message_len = sprintf(message, "[%.24s / ERR]: %s\n", ctime(&raw), buf);
      break;
  }

  fputs(message, stdout);
  write(fd, message, message_len);
}

void close_logs() {
  close(fd);
}
