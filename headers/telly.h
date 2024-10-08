#pragma once

#if defined(__linux__) && !defined(_GNU_SOURCE)
  #define _GNU_SOURCE
#endif
