#include <telly.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

static struct Configuration *_conf = NULL;
static int fd = -1;

// Also, use it as default maximum log length
static uint16_t block_size = 4096;

char **lines;
static int32_t log_lines = 0;

bool initialize_logs() {
  _conf = get_server_configuration();
  if ((fd = open_file(_conf->log_file, 0)) == -1) return false;

  struct stat sostat;
  stat(_conf->log_file, &sostat);

  const off_t file_size = sostat.st_size;
  block_size = sostat.st_blksize;

  if (_conf->max_log_lines != -1) {
    const uint32_t max_log_size = (block_size + 1);
    lines = malloc(_conf->max_log_lines * sizeof(char *));

    for (int32_t i = 0; i < _conf->max_log_lines; ++i) {
      lines[i] = malloc(max_log_size);
    }

    if (file_size != 0) {
      char *block;
      if (posix_memalign((void **) &block, block_size, block_size) != 0) return false;

      uint16_t latest = 0;
      uint16_t count = 0;
      char **buf = &lines[0];

      while ((count = read(fd, block, block_size))) {
        uint32_t i = 0;

        INSIDE_LOOP:
        while (i < count) {
          const char c = block[i];

          if (c == '\n') {
            memcpy(*buf, (block + latest), (i - latest + 1));
            (*buf)[i - latest + 1] = '\0';
            log_lines += 1;
            buf = &lines[log_lines];
            latest = i + 1;
          }

          i += 1;
        }

        if (count == latest) {
          latest = 0;
        } else {
          memcpy(*buf, (block + latest), (count - latest));

          const uint16_t old_count = count;
          count = read(fd, block, block_size);
          i = 0;

          while (i < count) {
            const char c = block[i];

            if (c == '\n') {
              memcpy(*buf + (old_count - latest), block, (i + 1));
              (*buf)[old_count - latest + i + 1] = '\0';
              log_lines += 1;
              buf = &lines[log_lines];
              i += 1;
              latest = i;
              goto INSIDE_LOOP;
            }

            i += 1;
          }
        }
      }

      free(block);
    }
  }

  return true;
}

void write_log(enum LogLevel level, const char *fmt, ...) {
  struct Configuration conf = _conf ? *_conf : get_default_configuration();
  const uint8_t check = (conf.allowed_log_levels & level);

  char time_text[21];
  generate_date_string(time_text, time(NULL));

  const uint32_t buf_size = block_size + 1;
  char buf[buf_size];
  va_list args;

  va_start(args, fmt);
  vsnprintf(buf, buf_size, fmt, args);
  va_end(args);

  uint32_t message_len = 0;
  char message[block_size + 34];

  FILE *stream;

  switch (check) {
    case LOG_INFO:
      stream = stdout;
      message_len = sprintf(message, "[%s / INFO] | %s\n", time_text, buf);
      break;

    case LOG_WARN:
      stream = stdout;
      message_len = sprintf(message, "[%s / WARN] | %s\n", time_text, buf);
      break;

    case LOG_ERR:
      stream = stderr;
      message_len = sprintf(message, "[%s / ERR]  | %s\n", time_text, buf);
      break;

    case LOG_DBG:
      stream = stdout;
      message_len = sprintf(message, "[%s / DBG]  | %s\n", time_text, buf);
      break;

    default:
      return;
  }

  fputs(message, stream);

  if (fd != -1) {
    if (conf.max_log_lines == -1) {
      log_lines += 1;

      if (log_lines != 1) lines = realloc(lines, log_lines * sizeof(char *));
      else lines = malloc(sizeof(char *));

      const uint32_t size = (block_size + 41);
      const int32_t at = (log_lines - 1);
      lines[at] = malloc(size);
      memcpy(lines[at], message, (message_len + 1));
    } else {
      if (log_lines == conf.max_log_lines) {
        const int32_t at = (log_lines - 1);
        char *line = lines[0];

        memcpy(lines, lines + 1, (log_lines - 1) * sizeof(char *));
        memcpy(line, message, (message_len + 1));
        lines[at] = line;
      } else {
        memcpy(lines[log_lines], message, (message_len + 1));
        log_lines += 1;
      }
    }
  }
}

void save_and_close_logs() {
  int status;

  if (fd != -1) {
    char *buf;

    if (posix_memalign((void **) &buf, block_size, block_size) == 0) {
      memset(buf, 0, block_size);

      off_t length = 0;
      uint16_t at = 0;
      lseek(fd, 0, SEEK_SET);

      for (int32_t i = 0; i < log_lines; ++i) {
        const char *line = lines[i];
        const uint32_t len = strlen(line);

        if ((at + len) <= block_size) {
          memcpy(buf + at, line, len);
          at += len;
        } else {
          const uint16_t remaining = (block_size - at);
          memcpy(buf + at, line, remaining);
          length += write(fd, buf, block_size);

          memcpy(buf, line + remaining, (at = (len - remaining)));
        }
      }

      if (at != 0) {
        if ((status = write(fd, buf, block_size)) == -1) {
          free(buf);
          write_log(LOG_WARN, "There was an error when writing logs to file.");
          return;
        }

        free(buf);

        if (status != -1 && (status = ftruncate(fd, length + at)) == -1) {
          write_log(LOG_WARN, "There was an error when writing logs to file.");
          return;
        }
      } else {
        if ((status = ftruncate(fd, length)) == -1) {
          free(buf);
          write_log(LOG_WARN, "There was an error when writing logs to file.");
          return;
        }
      }
    }

    if (_conf->max_log_lines != -1) {
      for (int32_t i = 0; i < _conf->max_log_lines; ++i) {
        free(lines[i]);
      }
    } else {
      for (int32_t i = 0; i < log_lines; ++i) {
        free(lines[i]);
      }
    }

    free(lines);

    if (status != -1 && (status = lockf(fd, F_ULOCK, 0)) == -1) {
      write_log(LOG_WARN, "There was an error when writing logs to file. The lock cannot be removed.");
    }

    close(fd);
  }
}
