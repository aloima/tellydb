#include <telly.h>

#define IO_QUEUE_SIZE 512

typedef struct {
  enum IOOpType type;
  Client *client;
  string_t to_write;
} IOOperation;

IOThread *io_threads = NULL;
static int64_t io_thread_count = 0;
static sigset_t set;

void *io_thread_procedure(void *arg);

IOThread *get_io_threads() {
  return io_threads;
}

int64_t get_io_thread_count() {
  return io_thread_count;
}

int create_io_threads() {
  GASSERT(sigemptyset(&set), ==, 0);
  GASSERT(sigaddset(&set, SIGINT), ==, 0);
  GASSERT(sigaddset(&set, SIGTERM), ==, 0);

  io_thread_count = max(sysconf(_SC_NPROCESSORS_ONLN) - 1, 2);
  io_threads = malloc(io_thread_count * sizeof(IOThread));
  if (io_threads == NULL) return -1;

  int64_t succeed = 0;

  while (succeed != io_thread_count) {
    IOThread *io_thread = &io_threads[succeed];

    event_notifier_t *notifier = (io_thread->notifier = create_notifier());
    ThreadQueue *queue = (io_thread->queue = create_tqueue(IO_QUEUE_SIZE, sizeof(IOOperation), alignof(IOOperation)));

    char *buf = (io_thread->buf = malloc(RESP_BUF_SIZE));
    Arena *ucmd_arena = (io_thread->ucmd_arena = arena_create(INITIAL_UNKNOWN_COMMAND_ARENA_SIZE));
    Arena *resp_arena = (io_thread->resp_arena = arena_create(INITIAL_RESP_ARENA_SIZE));

    if (notifier == NULL || queue == NULL || buf == NULL || ucmd_arena == NULL || resp_arena == NULL) {
      CLEANUP_THREAD:
      if (notifier) destroy_notifier(notifier);
      if (queue) free_tqueue(queue);

      if (buf) free(buf);
      if (ucmd_arena) arena_destroy(ucmd_arena);
      if (resp_arena) arena_destroy(resp_arena);

      break;
    }

    atomic_init(&io_thread->status, IO_THREAD_ACTIVE);

    const int code = pthread_create(&io_thread->thread, NULL, io_thread_procedure, io_thread);
    if (code == EAGAIN) goto CLEANUP_THREAD;

    GASSERT(pthread_detach(io_thread->thread), ==, 0);
    succeed += 1;
  }

  return succeed;
}

void send_destroy_signal_to_io_threads() {
  for (int64_t i = 0; i < io_thread_count; ++i) {
    atomic_store_explicit(&io_threads[i].status, IO_THREAD_PENDING_DESTROY, memory_order_relaxed);
    signal_notifier(io_threads[i].notifier, 1);
  }

  WAIT_DESTROYED:
  usleep(10);
  for (int64_t i = 0; i < io_thread_count; ++i) {
    if (atomic_load_explicit(&io_threads[i].status, memory_order_acquire) != IO_THREAD_DESTROYED)
      goto WAIT_DESTROYED;
  }

  free(io_threads);
}

int add_io_request(const enum IOOpType type, Client *client, string_t to_write) {
  if (client->id == -1) return -1;

  int thread_idx = (client->id % io_thread_count);
  IOThread *selected = &io_threads[thread_idx];

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

void *io_thread_procedure(void *arg) {
  GASSERT(pthread_sigmask(SIG_BLOCK, &set, NULL), ==, 0);

  IOThread *thread = (IOThread *) arg;
  int added = -1, efd = -1;

  efd = CREATE_EVENTFD();
  if (efd == -1) goto DESTROY;

  const int fd = get_notifier(thread->notifier);

  event_t event;
  CREATE_EVENT(event, fd);

  added = ADD_EVENT(efd, fd, event);
  if (added == -1) goto DESTROY;

  // There is exactly one fd/notifier
  event_t events[1];

  while (true) {
    WAIT_EVENTS(efd, events, 1, -1);
    if (atomic_load_explicit(&thread->status, memory_order_relaxed) == IO_THREAD_PENDING_DESTROY) break;

    const uint64_t count = consume_notifier(thread->notifier);

    for (uint64_t i = 0; i < count; ++i) {
      IOOperation op;
      if (!pop_tqueue(thread->queue, &op)) break;

      Client *client = op.client;
      if (client->id == -1) continue;

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
    }
  }

DESTROY:
  if (added != -1) {
    event_t ev;
    PREPARE_REMOVING_EVENT(ev, fd);
    REMOVE_EVENT(efd, fd);
  }

  if (efd != -1) close(efd);

  destroy_notifier(thread->notifier);
  free_tqueue(thread->queue);

  free(thread->buf);
  arena_destroy(thread->resp_arena);
  arena_destroy(thread->ucmd_arena);

  atomic_store_explicit(&thread->status, IO_THREAD_DESTROYED, memory_order_release);
  return NULL;
}
