#include <telly.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

static Config *_conf = NULL;
static int fd = -1;

// Also, use it as default maximum log length
static uint16_t block_size = 4096;

struct ThreadQueue *lines;

bool initialize_logs() {
  _conf = server->conf;
  if ((fd = open_file(_conf->log_file, 0)) == -1) return false;

  struct stat sostat;
  stat(_conf->log_file, &sostat);

  const off_t file_size = sostat.st_size;
  block_size = sostat.st_blksize;

  const uint32_t size = (block_size + 41);

  if (_conf->max_log_lines != -1) {
    const uint32_t max_log_size = (block_size + 1);
    lines = create_tqueue(_conf->max_log_lines * 2, sizeof(char *), _Alignof(char *));

    if (file_size != 0) {
      char *block;
      if (posix_memalign((void **) &block, block_size, block_size) != 0) return false;

      uint16_t latest = 0;
      uint16_t count = 0;
      char *buf = malloc(size);

      while ((count = read(fd, block, block_size))) {
        uint32_t i = 0;

INSIDE_LOOP:
        while (i < count) {
          const char c = block[i];

          if (c == '\n') {
            memcpy(buf, (block + latest), (i - latest + 1));
            buf[i - latest + 1] = '\0';
            latest = i + 1;

            push_tqueue(lines, &buf);
            buf = malloc(size);
          }

          i += 1;
        }

        if (count == latest) {
          latest = 0;
        } else {
          memcpy(buf, (block + latest), (count - latest));

          const uint16_t old_count = count;
          count = read(fd, block, block_size);
          i = 0;

          while (i < count) {
            const char c = block[i];

            if (c == '\n') {
              memcpy(buf + (old_count - latest), block, (i + 1));
              buf[old_count - latest + i + 1] = '\0';

              push_tqueue(lines, &buf);
              buf = malloc(size);

              i += 1;
              latest = i;
              goto INSIDE_LOOP;
            }

            i += 1;
          }
        }
      }

      free(buf);
      free(block);
    }
  }

  return true;
}

void write_log(enum LogLevel level, const char *fmt, ...) {
  Config *conf = _conf ?: get_default_config();
  const uint8_t check = (conf->allowed_log_levels & level);

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
  if (fd == -1) return;

  const uint32_t size = (block_size + 41);

  if (conf->max_log_lines == -1) {
    char *line = malloc(size);
    memcpy(line, message, (message_len + 1));

    push_tqueue(lines, &line);
  } else {
    if (estimate_tqueue_size(lines) >= conf->max_log_lines) {
      char *line;
      pop_tqueue(lines, &line);
      free(line);
    }

    char *line = malloc(size);
    memcpy(line, message, (message_len + 1));
    push_tqueue(lines, &line);
  }
}

void save_and_close_logs() {
  int status = 0;
  if (fd == -1) return;

  char *buf;
  if (posix_memalign((void **) &buf, block_size, block_size) != 0) goto CLEANUP;
  memset(buf, 0, block_size);

  off_t length = 0;
  uint16_t at = 0;
  lseek(fd, 0, SEEK_SET);

  while (estimate_tqueue_size(lines) != 0) {
    char *line;
    pop_tqueue(lines, &line);

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

    free(line);
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

  assert(estimate_tqueue_size(lines) == 0);

CLEANUP:
  free_tqueue(lines);

  if (status != -1 && (status = lockf(fd, F_ULOCK, 0)) == -1) {
    write_log(LOG_WARN, "There was an error when writing logs to file. The lock cannot be removed.");
  }

  close(fd);
}
