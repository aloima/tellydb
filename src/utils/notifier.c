#include <telly.h>

#include <stdlib.h>
#include <stdint.h>

#include <fcntl.h>
#include <unistd.h>

#if defined(__linux__)
  #include <sys/eventfd.h>

  event_notifier_t *create_notifier() {
    event_notifier_t *notifier = malloc(sizeof(event_notifier_t));
    if (VERY_UNLIKELY(notifier == NULL)) return NULL;

    notifier->efd = eventfd(0, EFD_NONBLOCK);
    if (VERY_UNLIKELY(notifier->efd == -1)) {
      free(notifier);
      return NULL;
    }

    return notifier;
  }

  void signal_notifier(event_notifier_t *notifier, uint64_t n) {
    if (VERY_UNLIKELY(notifier == NULL)) return;
    write(notifier->efd, &n, sizeof(n));
  }

  uint64_t consume_notifier(event_notifier_t *notifier) {
    if (VERY_UNLIKELY(notifier == NULL)) return -1;

    uint64_t val = 0;
    read(notifier->efd, &val, sizeof(val));

    return val;
  }

  int get_notifier(event_notifier_t *notifier) {
    return notifier->efd;
  }

  void destroy_notifier(event_notifier_t *notifier) {
    close(notifier->efd);
    free(notifier);
  }
#elif defined(__APPLE__)
  // Entire apple base is untested
  event_notifier_t *create_notifier() {
    event_notifier_t *notifier = malloc(sizeof(event_notifier_t));
    if (VERY_UNLIKELY(notifier == NULL)) return NULL;

    if (VERY_UNLIKELY(pipe(notifier->fds) == -1)) {
      free(notifier);
      return NULL;
    }

    int flags = fcntl(notifier->fds[0], F_GETFL, 0);
    fcntl(notifier->fds[0], F_SETFL, flags | O_NONBLOCK);

    return notifier;
  }

  void signal_notifier(event_notifier_t *notifier, uint64_t n) {
    if (VERY_UNLIKELY(notifier == NULL)) return;
    write(notifier->fds[1], &n, sizeof(n));
  }

  uint64_t consume_notifier(event_notifier_t *notifier) {
    if (VERY_UNLIKELY(notifier == NULL)) return -1;

    uint64_t result = 0;
    uint64_t val[128];
    int bytes;

    while ((bytes = read(notifier->fds[0], val, sizeof(val))) > 0) {
      const int count = (bytes / sizeof(val[0]));

      for (int i = 0; i < count; ++i) {
        result += val[i];
      }
    }

    return result;
  }

  int get_notifier(event_notifier_t *notifier) {
    return notifier->fds[0];
  }

  void destroy_notifier(event_notifier_t *notifier) {
    close(notifier->fds[0]);
    close(notifier->fds[1]);
    free(notifier);
  }
#endif
