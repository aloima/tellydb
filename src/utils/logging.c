#include "../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <time.h>

void write_log(const char *message, const enum LogLevel level, const uint8_t allowed_log_levels) {
  const uint8_t check = allowed_log_levels & level;
  time_t raw;
  time(&raw);

  switch (check) {
    case LOG_INFO:
      fprintf(stdout, "[%.24s / INFO]: %s\n", ctime(&raw), message);
      break;

    case LOG_WARN:
      fprintf(stdout, "[%.24s / WARN]: %s\n", ctime(&raw), message);
      break;

    case LOG_ERR:
      fprintf(stdout, "[%.24s / ERR]: %s\n", ctime(&raw), message);
      break;
  }
}
