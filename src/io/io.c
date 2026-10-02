#include <telly.h>
#include "io.h"

sigset_t io_sigset;

static inline int initialize_thread(IOThread *thread) {
  thread->notifier = create_notifier();
  thread->queue = create_tqueue(IO_QUEUE_SIZE, sizeof(IOOperation), alignof(IOOperation));
  thread->ucmd_arena = arena_create(INITIAL_UNKNOWN_COMMAND_ARENA_SIZE);

  if (thread->notifier == NULL || thread->queue == NULL || thread->ucmd_arena == NULL) {
    cleanup_thread(thread);
    return -1;
  }

  atomic_init(&thread->status, IO_THREAD_ACTIVE);
  return 0;
}

int create_io_threads() {
  ASSERT(sigemptyset(&io_sigset), ==, 0);
  ASSERT(sigaddset(&io_sigset, SIGINT), ==, 0);
  ASSERT(sigaddset(&io_sigset, SIGTERM), ==, 0);

  uint32_t thread_count = max(sysconf(_SC_NPROCESSORS_ONLN) - 1, 2);
  IOThread *threads = malloc(thread_count * sizeof(IOThread));
  if (threads == NULL) return -1;

  int64_t succeed = 0;

  while (succeed != thread_count) {
    IOThread *thread = &threads[succeed];
    if (initialize_thread(thread) == -1) break;

    const int code = pthread_create(&thread->thread, NULL, io_thread_procedure, thread);

    if (code == EAGAIN) {
      cleanup_thread(thread);
      break;
    }

    ASSERT(pthread_detach(thread->thread), ==, 0);
    succeed += 1;
  }

  server->io_threads = threads;
  server->io_thread_count = thread_count;

  return succeed;
}

int add_io_request(const IOOpType type, Client *client, string_t to_write) {
  if (client->id == -1) return -1;

  int thread_idx = (client->id % server->io_thread_count);
  IOThread *selected = &server->io_threads[thread_idx];

  IOOperation op = {
    .type = type,
    .client = client,

    // In transaction thread, each `to_write` value will be stored independenly already.
    // Storing in extra layer (here) is redundant.
    .to_write = to_write
  };

  push_tqueue(selected->queue, &op);
  signal_notifier(selected->notifier, 1);

  return thread_idx;
}