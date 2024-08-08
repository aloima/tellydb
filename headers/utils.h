#include <stddef.h>
#include <stdint.h>

#ifndef UTILS_H
  #define UTILS_H

  typedef struct String {
    char *value;
    size_t len;
  } string_t;

  void set_string(string_t *data, char *value, int32_t len);
#endif
