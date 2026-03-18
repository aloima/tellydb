#pragma once

#include <stdint.h>

enum LogLevel : uint8_t {
  LOG_INFO = 0b0001,
  LOG_WARN = 0b0010,
  LOG_ERR  = 0b0100,
  LOG_DBG  = 0b1000,
};

int initialize_logs();
void write_log(enum LogLevel level, const char *fmt, ...);
void save_and_close_logs();
