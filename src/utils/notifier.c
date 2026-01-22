#include <telly.h>

#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

#if defined(__linux__)
  #include <sys/eventfd.h>

  event_notifier_t *create_notifier() {
    event_notifier_t *notifier = malloc(sizeof(event_notifier_t));
    if (notifier == NULL) return NULL;

    *notifier = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
    if (*notifier == -1) {
      free(notifier);
      return NULL;
    }

    return notifier;
  }

  void signal_notifier(event_notifier_t *notifier) {
    if (notifier == NULL) return;
    uint64_t val = 1;
    write(*notifier, &val, sizeof(val));
  }

  void consume_notifier(event_notifier_t *notifier) {
    if (notifier == NULL) return;
    uint64_t val;
    read(*notifier, &val, sizeof(val));
  }
#elif defined(__APPLE__)
  typedef int event_notifier_t[2];

  event_notifier_t *create_notifier() {
    event_notifier_t *notifier = malloc(sizeof(event_notifier_t));
    if (notifier == NULL) return NULL;

    if (pipe(*notifier) == -1) {
      free(notifier);
      return NULL;
    }

    int flags = fcntl((*notifier)[0], F_GETFL, 0);
    fcntl((*notifier)[0], F_SETFL, flags | O_NONBLOCK);
  }

  void signal_notifier(event_notifier_t *notifier) {
    if (notifier == NULL) return;

    char byte = 1;
    write((*notifier)[1], &byte, 1);
  }

  void consume_notifier(event_notifier_t *notifier) {
    if (notifier == NULL) return;

    char buf[256];
    while (read((*notifier)[0], buf, sizeof(buf)) > 0);
  }
#endif
