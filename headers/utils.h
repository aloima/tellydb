#pragma once

#include "config.h"

#include <string.h> // IWYU pragma: keep
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include <sys/syscall.h> // IWYU pragma: keep
#include <unistd.h> // IWYU pragma: keep

#define streq(s1, s2) (strcmp((s1), (s2)) == 0)
#define IS_IN_PROCESS() (getpid() == syscall(SYS_gettid))

struct LinkedListNode {
  void *data;
  void *next;
};

typedef struct String {
  char *value;
  uint32_t len;
} string_t;

void to_uppercase(string_t src, char *dst);

#define EMPTY_STRING() ((string_t) {"", 0})
#define CREATE_STRING(value, len) ((string_t) {value, len})

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

void memcpy_aligned(void *restrict dest, const void *restrict src, size_t n);

const bool is_integer(const char *value);
const bool is_double(const char *value);
const uint8_t ltoa(const int64_t value, char *dst);
const uint8_t get_digit_count(const uint64_t value);
const uint8_t get_byte_count(const uint64_t value);

void generate_random_string(char *dest, size_t length);
void generate_date_string(char *text, const time_t value);

int open_file(const char *file, int flags);

uint64_t hash(char *key, uint32_t length);
