#include "utils.h"

#include <stdint.h>

#ifndef DATABASE_H
  #define DATABASE_H

  #define TELLY_NULL 1
  #define TELLY_INT 2
  #define TELLY_STR 3
  #define TELLY_BOOL 4

  struct KVPair {
    string_t key;

    union {
      string_t string;
      int integer;
      bool boolean;
      void *null;
    } value;

    uint32_t type;
  };
#endif
