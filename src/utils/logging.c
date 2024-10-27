#include "../../headers/utils.h"
#include "../../headers/config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include <fcntl.h>
#include <unistd.h>

static struct Configuration *conf;
static int fd = -1;
static int32_t log_lines = 0;

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
    fd = open(conf->log_file, (O_RDWR | O_CREAT | O_DIRECT), (S_IRUSR | S_IWUSR));
  #elif defined(__APPLE__)
    fd = open(conf->log_file, (O_RDWR | O_CREAT), (S_IRUSR | S_IWUSR));

    if (fcntl(fd, F_NOCACHE, 1) == -1) {
      write_log(LOG_ERR, "Cannot deactive file caching for database file.");
    }
  #endif

  if (conf->max_log_lines != -1) {
    char c;

    while (read(fd, &c, 1) != 0) {
      if (c == '\n') {
        log_lines += 1;
      }
    }
  }
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

  if (conf->max_log_lines != -1) {
    if (log_lines >= conf->max_log_lines) {
      const off_t size = lseek(fd, 0, SEEK_END);

      lseek(fd, 0, SEEK_SET);
      char c;

      while (read(fd, &c, 1) != 0) {
        if (c == '\n') {
          if (log_lines == conf->max_log_lines) {
            const off_t at = lseek(fd, 0, SEEK_CUR);
            const off_t data_len = size - at;
            char *data = malloc(data_len);
            read(fd, data, data_len);
            lseek(fd, 0, SEEK_SET);
            write(fd, data, data_len);

            write(fd, message, message_len);
            ftruncate(fd, data_len + message_len);
            free(data);

            break;
          } else {
            log_lines -= 1;
          }
        }
      }
    } else {
      write(fd, message, message_len);
      log_lines += 1;
    }
  } else {
    write(fd, message, message_len);
  }
}

void close_logs() {
  close(fd);
}
