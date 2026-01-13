#include <telly.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

static int fd = -1;
static off_t new_size = 0;
#define LOG_LENGTH 4096

struct ThreadQueue *lines;

// TODO: handle max_log_liens == -1
bool initialize_logs() {
  const Config *conf = server->conf;
  if (conf->log_file[0] == '\0') return true;
  if ((fd = open_file(conf->log_file, 0)) == -1) return false;

  struct stat sostat;
  stat(conf->log_file, &sostat);

  const off_t size = sostat.st_size;
  new_size += size;
  lines = create_tqueue(conf->max_log_lines * 2, sizeof(char *), alignof(typeof(char *)));

  if (size != 0) {
    char *data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    off_t old_at = 0;
    off_t at = 0;

    while (data[at] != '\0') {
      if (data[at] != '\n') {
        at += 1;
        continue;
      }

      const off_t line_len = (at - old_at + 1); // line + '\n'
      char *line = malloc(line_len + 1);
      memcpy(line, data + old_at, line_len * sizeof(char));
      line[line_len] = '\0';

      push_tqueue(lines, &line);
      at += 1;
      old_at = at;
    }

    munmap(data, size);
  }

  return true;
}

void write_log(enum LogLevel level, const char *fmt, ...) {
  Config *conf = server->conf ?: get_default_config();
  const uint8_t check = (conf->allowed_log_levels & level);

  char time_text[21];
  generate_date_string(time_text, time(NULL));

  const uint32_t buf_size = LOG_LENGTH + 1;
  char buf[buf_size];
  va_list args;

  va_start(args, fmt);
  vsnprintf(buf, buf_size, fmt, args);
  va_end(args);

  uint32_t message_len = 0;
  char message[LOG_LENGTH + 34];

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

  if (conf->max_log_lines == -1) {
    char *line = malloc(message_len + 1);
    memcpy(line, message, (message_len + 1));

    push_tqueue(lines, &line);
  } else {
    if (estimate_tqueue_size(lines) >= conf->max_log_lines) {
      char *line;
      pop_tqueue(lines, &line);
      new_size -= strlen(line);
      free(line);
    }

    char *line = malloc(message_len + 1);
    memcpy(line, message, (message_len + 1));
    push_tqueue(lines, &line);
    new_size += message_len;
  }
}

#define CHECK_ERROR(ERROR_CODE, message) \
  case (ERROR_CODE): \
    fprintf(stderr, (message)); \
    return

void save_and_close_logs() {
  if (fd == -1) return;
  if (ftruncate(fd, new_size) == -1) {
    if (errno == EIO) fprintf(stderr, "Cannot write logs to file, truncate I/O error.");
    return;
  }

  off_t written = 0;
  char *map = mmap(NULL, new_size + 1, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED) {
    switch (errno) {
      CHECK_ERROR(ENOMEM, "Cannot write logs to file, out of memory.");
      CHECK_ERROR(EAGAIN, "Cannot write logs to file, locked log file.");
      CHECK_ERROR(ENFILE, "Cannot write logs to file, exceeded max opened files.");
      CHECK_ERROR(ENODEV, "Cannot write logs to file, file system does not support memory mapping.");
      CHECK_ERROR(EPERM,  "Cannot write logs to file, permission error.");
    }
  }

  while (estimate_tqueue_size(lines) != 0) {
    char *line;
    if (!pop_tqueue(lines, &line)) break;

    const uint32_t len = strlen(line);

    memcpy(map + written, line, len);
    written += len;

    free(line);
  }

  map[new_size] = '\0';
  assert(estimate_tqueue_size(lines) == 0);

CLEANUP:
  free_tqueue(lines);
  msync(map, new_size + 1, MS_ASYNC);

  munmap(map, new_size + 1);
  if (map == MAP_FAILED) {
    switch (errno) {
      CHECK_ERROR(ENOMEM, "Cannot write logs to file, out of memory.");
      CHECK_ERROR(EAGAIN, "Cannot write logs to file, locked log file.");
      CHECK_ERROR(ENFILE, "Cannot write logs to file, exceeded max opened files.");
      CHECK_ERROR(ENODEV, "Cannot write logs to file, file system does not support memory mapping.");
      CHECK_ERROR(EPERM,  "Cannot write logs to file, permission error.");
    }
  }

  close(fd);
}
