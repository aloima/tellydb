#pragma once

#include <string.h> // IWYU pragma: keep
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdatomic.h>

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

enum ThreadQueueState : uint8_t {
  TQ_EMPTY,
  TQ_STORING,
  TQ_STORED
};

struct ThreadQueue {
  _Atomic uint64_t at;
  _Atomic uint64_t end;

  _Atomic enum ThreadQueueState *states;
  void *data;
  uint64_t capacity;
  uint64_t type;
};

struct ThreadQueue *create_tqueue(const uint64_t capacity, const uint64_t size, const uint64_t align);
void free_tqueue(struct ThreadQueue *queue);

uint64_t calculate_tqueue_size(const struct ThreadQueue *queue);
void *push_tqueue(struct ThreadQueue *queue, void *value);
void *pop_tqueue(struct ThreadQueue *queue);
void *get_tqueue_value(struct ThreadQueue *queue, const uint64_t idx);

enum TellyTypes {
  TELLY_NULL,
  TELLY_INT,
  TELLY_DOUBLE,
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

bool initialize_logs();
void write_log(enum LogLevel level, const char *fmt, ...);
void save_and_close_logs();

void memcpy_aligned(void *restrict dest, const void *restrict src, size_t n);

bool try_parse_integer(const char *value);
bool try_parse_double(const char *value);
uint8_t ltoa(const int64_t value, char *dst);
uint8_t get_digit_count(const uint64_t value);
uint8_t get_bit_count(const uint64_t value);
uint8_t get_byte_count(const uint64_t value);

void generate_random_string(char *dest, size_t length);
void generate_date_string(char *text, const time_t value);

int open_file(const char *file, int flags);

uint64_t hash(char *key, uint32_t length);
