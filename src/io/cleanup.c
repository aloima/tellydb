#include <telly.h>
#include "io.h"

void cleanup_thread(IOThread *thread) {
  event_notifier_t *notifier = thread->notifier;
  ThreadQueue *queue = thread->queue;
  Arena *ucmd_arena = thread->ucmd_arena;
  
  if (notifier) destroy_notifier(notifier);
  if (queue) free_tqueue(queue);
  if (ucmd_arena) arena_destroy(ucmd_arena);
}

void destroy_thread(IOThread *thread, int added, int efd, int fd) {
  if (added != -1) {
    event_t ev;
    PREPARE_REMOVING_EVENT(ev, fd);
    REMOVE_EVENT(efd, fd);
  }

  if (efd != -1) {
    close(efd);
  }

  cleanup_thread(thread);
  atomic_store_explicit(&thread->status, IO_THREAD_DESTROYED, memory_order_release);
}

void send_destroy_signal_to_io_threads() {
  IOThread *threads = server->io_threads;
  const uint32_t count = server->io_thread_count;

  for (uint32_t i = 0; i < count; ++i) {
    atomic_store_explicit(&threads[i].status, IO_THREAD_PENDING_DESTROY, memory_order_relaxed);
    signal_notifier(threads[i].notifier, 1);
  }

  for (uint32_t i = 0; i < count; ++i) {
    while (atomic_load_explicit(&threads[i].status, memory_order_acquire) != IO_THREAD_DESTROYED) {
      ASSERT(tsleep(10), ==, 0);
    }
  }

  free(threads);
}