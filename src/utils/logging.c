#include <telly.h>

static int fd = -1;
static _Atomic(off_t) new_size;
#define LOG_LENGTH 4096

static struct ThreadQueue *lines;

int initialize_logs() {
  const Config *conf = server->conf;
  if (conf->log_file[0] == '\0') return 0;
  if (conf->max_log_lines < 0 && conf->max_log_lines != -1) return -1;

  if (conf->max_log_lines == -1) {
    if ((fd = open_file(conf->log_file, O_APPEND)) == -1) return -1;
    return 0;
  } else {
    if ((fd = open_file(conf->log_file, 0)) == -1) return -1;
  }

  // All errors from stat() method are already handled by open_file() method
  struct stat sostat;
  GASSERT(stat(conf->log_file, &sostat), !=, -1);

  const off_t size = sostat.st_size;
  atomic_init(&new_size, size);

  lines = create_tqueue(conf->max_log_lines, sizeof(char *), alignof(char *));
  if (lines == NULL) {
    write_log(LOG_ERR, "Cannot initialized logs, out of memory.");
    return -1;
  }

  // Specified log file does not contain any lines, so there is no need for
  // taking unexisted lines into lines thread queue
  if (size == 0) return 0;

  #define CHECK_ERROR(ERROR_CODE, message) \
    case (ERROR_CODE): \
      fprintf(stderr, (message)); \
      return -1

  char *data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data == MAP_FAILED) {
    switch (errno) {
      CHECK_ERROR(ENOMEM, "Cannot initialized logs, out of memory.");
      CHECK_ERROR(ENFILE, "Cannot initialized logs, exceeded max opened files.");
      CHECK_ERROR(ENODEV, "Cannot initialized logs, file system does not support memory mapping.");
      CHECK_ERROR(EPERM,  "Cannot initialized logs, permission error.");
    }
  }

  #undef CHECK_ERROR

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

  GASSERT(munmap(data, size), ==, 0);
  return 0;
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
      if (server) {
        server->last_error_at = time(NULL);
        server->status = SERVER_STATUS_ERROR;
      }
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
    write(fd, message, message_len);
  } else {
    if (estimate_tqueue_size(lines) >= conf->max_log_lines) {
      char *line;
      pop_tqueue(lines, &line);
      atomic_fetch_sub_explicit(&new_size, strlen(line), memory_order_relaxed);
      free(line);
    }

    char *line = malloc(message_len + 1);
    memcpy(line, message, (message_len + 1));
    push_tqueue(lines, &line);
    atomic_fetch_add_explicit(&new_size, message_len, memory_order_relaxed);
  }
}

void save_and_close_logs() {
  if (fd == -1 || server->conf->max_log_lines == -1) return;
  const off_t size = atomic_load_explicit(&new_size, memory_order_consume);

  if (ftruncate(fd, size) == -1) {
    if (errno == EIO) fprintf(stderr, "Cannot write logs to file, truncate I/O error.");
    return;
  }

  #define CHECK_ERROR(ERROR_CODE, message) \
    case (ERROR_CODE): \
      fprintf(stderr, (message)); \
      return

  off_t written = 0;
  char *map = mmap(NULL, size + 1, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
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

  map[size] = '\0';
  GASSERT(estimate_tqueue_size(lines), ==, 0);

CLEANUP:
  free_tqueue(lines);
  msync(map, size + 1, MS_ASYNC);

  munmap(map, size + 1);
  GASSERT(close(fd), ==, 0);

  #undef CHECK_ERROR
}
