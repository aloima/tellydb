#pragma once

#if defined(__linux__)
  typedef int event_notifier_t;
#elif defined(__APPLE__)
  typedef int event_notifier_t[2];
#endif

event_notifier_t *create_notifier();
void signal_notifier(event_notifier_t *notifier);
void consume_notifier(event_notifier_t *notifier);
