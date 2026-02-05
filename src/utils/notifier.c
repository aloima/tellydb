#include <telly.h>

#include <stdlib.h>
#include <stdint.h>

#include <fcntl.h>
#include <unistd.h>

// TODO: writing uint64_t is mostly expensive, needed to be used get_byte_count writing
// avoid redundant zero byte writing

#if defined(__linux__)
  #include <sys/eventfd.h>

  event_notifier_t *create_notifier() {
    event_notifier_t *notifier = malloc(sizeof(event_notifier_t));
    if (notifier == NULL) return NULL;

    *notifier = eventfd(0, EFD_NONBLOCK);
    if (*notifier == -1) {
      free(notifier);
      return NULL;
    }

    return notifier;
  }

  void signal_notifier(event_notifier_t *notifier, uint64_t n) {
    if (notifier == NULL) return;
    write(*notifier, &n, sizeof(n));
  }

  uint64_t consume_notifier(event_notifier_t *notifier) {
    if (notifier == NULL) return -1;

    uint64_t val;
    read(*notifier, &val, sizeof(val));

    return val;
  }

  int get_notifier(event_notifier_t *notifier) {
    return *notifier;
  }

  void destroy_notifier(event_notifier_t *notifier) {
    close(*notifier);
    free(notifier);
  }
#elif defined(__APPLE__)
  // Entire apple base is untested
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

  void signal_notifier(event_notifier_t *notifier, uint64_t n) {
    if (notifier == NULL) return;
    write((*notifier)[1], &n, sizeof(n));
  }

  uint64_t consume_notifier(event_notifier_t *notifier) {
    if (notifier == NULL) return;

    uint64_t result = 0;
    uint64_t val[128];
    int bytes;

    while ((bytes = read((*notifier)[0], val, sizeof(val))) > 0) {
      const int count = (bytes / sizeof(val[0]));

      for (int i = 0; i < count; ++i) {
        result += val[i];
      }
    }
  }

  int get_notifier(event_notifier_t *notifier) {
    return (*notifier)[0];
  }

  void destroy_notifier(event_notifier_t *notifier) {
    close((*notifier)[0]);
    close((*notifier)[1]);
    free(notifier);
  }
#endif
