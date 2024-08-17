#include "../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>

void write_log(const char *message, const enum LogLevel level, const uint8_t allowed_log_levels) {
  const uint8_t check = allowed_log_levels & level;

  switch (check) {
    case LOG_INFO:
      fprintf(stdout, "[INFO]: %s\n", message);
      break;

    case LOG_WARN:
      fprintf(stdout, "[WARN]: %s\n", message);
      break;

    case LOG_ERR:
      fprintf(stdout, "[ERR]: %s\n", message);
      break;
  }
}
