#pragma once

#include <stdint.h>
#include <stdatomic.h>

#if defined(__linux__)
  typedef struct {
    int efd;
  } event_notifier_t;
#elif defined(__APPLE__)
  typedef struct {
    int fds[2];
  } event_notifier_t;
#endif

event_notifier_t *create_notifier();
void signal_notifier(event_notifier_t *notifier, uint64_t n);
uint64_t consume_notifier(event_notifier_t *notifier);
int get_notifier(event_notifier_t *notifier);
void destroy_notifier(event_notifier_t *notifier);
