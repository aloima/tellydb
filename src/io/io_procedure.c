#include <telly.h>
#include "io.h"

static inline int catch_op_and_execute(IOThread *thread) {
  IOOperation op;
  if (!pop_tqueue(thread->queue, &op)) return -1;

  Client *client = op.client;
  if (client->id == -1) return 0;

  switch (op.type) {
    case IOOP_TERMINATE:
      terminate_connection(client);
      break;

    case IOOP_READ:
      read_command(thread, client);
      break;

    case IOOP_WRITE:
      write_to_socket(client, op.to_write.value, op.to_write.len);
      break;
  }

  return 0;
}

static inline int initialize_multiplexing(IOThread *thread, int *added, int *efd, int *fd) {
  int _added = -1, _efd = -1, _fd = -1;
  _efd = CREATE_EVENTFD();

  if (_efd == -1) {
    destroy_thread(thread, _added, _efd, _fd);
    return -1;
  }

  _fd = get_notifier(thread->notifier);

  event_t event;
  CREATE_EVENT(event, _fd);

  _added = ADD_EVENT(_efd, _fd, event);

  if (_added == -1) {
    destroy_thread(thread, _added, _efd, _fd);
    return -1;
  }

  *added = _added;
  *efd = _efd;
  *fd = _fd;

  return 0;
}

void *io_thread_procedure(void *arg) {
  ASSERT(pthread_sigmask(SIG_BLOCK, &io_sigset, NULL), ==, 0);

  IOThread *thread = (IOThread *) arg;
  int added = -1, efd = -1, fd = -1;

  if (initialize_multiplexing(thread, &added, &efd, &fd) == -1) {
    return NULL;
  }

  // There is exactly one fd/notifier
  event_t events[1];

  while (true) {
    TEMP_FAILURE_RETRY(WAIT_EVENTS(efd, events, 1, -1));
    if (atomic_load_explicit(&thread->status, memory_order_relaxed) == IO_THREAD_PENDING_DESTROY) break;

    const uint64_t count = consume_notifier(thread->notifier);

    for (uint64_t i = 0; i < count; ++i) {
      if (catch_op_and_execute(thread) == -1) break;
    }
  }

  destroy_thread(thread, added, efd, fd);
  return NULL;
}
