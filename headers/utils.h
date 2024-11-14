#pragma once

#include "config.h"

#include <string.h> // IWYU pragma: keep
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define streq(s1, s2) (strcmp((s1), (s2)) == 0)

void to_uppercase(char *in, char *out);

typedef struct String {
  char *value;
  uint32_t len;
} string_t;

enum TellyTypes {
  TELLY_NULL,
  TELLY_NUM,
  TELLY_STR,
  TELLY_BOOL,
  TELLY_HASHTABLE,
  TELLY_LIST
};

enum LogLevel {
  LOG_INFO = 0b001,
  LOG_WARN = 0b010,
  LOG_ERR  = 0b100,
};

bool initialize_logs(struct Configuration *config);
void write_log(enum LogLevel level, const char *fmt, ...);
void save_and_close_logs();

bool is_integer(const char *value);
uint32_t get_digit_count(int64_t number);
void number_pad(char *res, const uint32_t value);

void read_string_from_file(const int fd, string_t *string, const bool unallocated, const bool terminator);
void generate_random_string(char *dest, size_t length);
void generate_date_string(char *text, const time_t value);

int open_file(const char *file, int flags);

uint64_t hash(char *key);
