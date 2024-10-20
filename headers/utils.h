#pragma once

#include "config.h"

#include <string.h> // IWYU pragma: keep
#include <stdint.h>
#include <stdbool.h>

#define streq(s1, s2) (strcmp((s1), (s2)) == 0)

void to_uppercase(char *in, char *out);

typedef struct String {
  char *value;
  size_t len;
} string_t;

enum TellyTypes {
  TELLY_NULL,
  TELLY_NUM,
  TELLY_STR,
  TELLY_BOOL,
  TELLY_HASHTABLE,
  TELLY_LIST
};

typedef union {
  string_t string;
  long number;
  bool boolean;
  void *null;
  struct HashTable *hashtable;
  struct List *list;
} value_t;

enum LogLevel {
  LOG_INFO = 0b001,
  LOG_WARN = 0b010,
  LOG_ERR  = 0b100,
};

void initialize_logs(struct Configuration *config);
void write_log(enum LogLevel level, const char *fmt, ...);
void close_logs();

bool is_integer(const char *value);
uint32_t get_digit_count(int64_t number);
