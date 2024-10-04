#include "../../headers/telly.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>

#include <fcntl.h>
#include <unistd.h>

static struct Configuration *conf;
static int fd = -1;

static const char month_name[][4] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static void number_pad(char *res, const uint32_t value) {
  if (value < 10) {
    sprintf(res, "0%u", value);
  } else if (value < 100) {
    sprintf(res, "%u", value);
  }
}

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

  time_t rawtime = time(NULL);
  struct tm *tm = localtime(&rawtime);
  
  char date[3], mon[4], hour[3], min[3], sec[3];
  number_pad(date, tm->tm_mday);
  sprintf(mon, "%.3s", month_name[tm->tm_mon]);
  number_pad(hour, tm->tm_hour);
  number_pad(min, tm->tm_min);
  number_pad(sec, tm->tm_sec);

  char time_text[28]; // tm->tm_year is integer, not 4-digit
  sprintf(time_text, "%s %s %d %s:%s:%s", date, mon, 1900 + tm->tm_year, hour, min, sec);

  const uint32_t buf_size = conf->max_log_len + 1;
  char buf[buf_size];
  va_list args;

  va_start(args, fmt);
  vsnprintf(buf, buf_size, fmt, args);
  va_end(args);

  uint32_t message_len = conf->max_log_len + 41;
  char message[message_len + 1];

  switch (check) {
    case LOG_INFO:
      message_len = sprintf(message, "[%s / INFO] | %s\n", time_text, buf);
      break;

    case LOG_WARN:
      message_len = sprintf(message, "[%s / WARN] | %s\n", time_text, buf);
      break;

    case LOG_ERR:
      message_len = sprintf(message, "[%s / ERR]  | %s\n", time_text, buf);
      break;
  }

  fputs(message, stdout);
  write(fd, message, message_len);
}

void close_logs() {
  close(fd);
}
