#include <telly.h>

int tsleep(uint64_t microseconds) {
  struct timespec req, rem;

  req.tv_sec = microseconds / 1000000UL;
  req.tv_nsec = (microseconds % 1000000UL) * 1000UL;

  while (nanosleep(&req, &rem) == -1) {
    if (errno == EINTR) {
      req = rem;
    } else {
      return -1;
    }
  }

  return 0;
}