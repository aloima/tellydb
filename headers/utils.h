#include "config.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef UTILS_H
  #define UTILS_H

  typedef struct String {
    char *value;
    size_t len;
  } string_t;

  void set_string(string_t *data, char *value, int32_t len, bool unset);
  void to_uppercase(char *in, char *out);

  enum TellyTypes {
    TELLY_NULL = 1,
    TELLY_INT,
    TELLY_STR,
    TELLY_BOOL,
    TELLY_HASHTABLE,
    TELLY_LIST
  };

  typedef union {
    string_t string;
    int integer;
    bool boolean;
    void *null;
    struct HashTable *hashtable;
    struct List *list;
  } value_t;

  void write_value(int connfd, value_t value, enum TellyTypes type);

  enum LogLevel {
    LOG_INFO = 0b001,
    LOG_WARN = 0b010,
    LOG_ERR  = 0b100,
  };

  void initialize_logs(struct Configuration *config);
  void write_log(enum LogLevel level, const char *fmt, ...);

  bool is_integer(const char *value);
  uint32_t get_digit_count(int32_t number);
#endif
