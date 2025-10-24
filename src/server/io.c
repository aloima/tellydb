#include <telly.h>

#include <stdbool.h>
#include <stdatomic.h>

#include <pthread.h>
#include <semaphore.h>

static struct IORequest *memory = NULL;
_Atomic(uint64_t) at, end;
static sem_t sem;

static void *io_worker(void *_) {
  while (true) {
    sem_wait(&sem);

    struct IORequest request = memory[atomic_fetch_add_explicit(&at, 1, memory_order_relaxed)];

    switch (request.type) {
      case IO_READ:
        _read(request.client, request.data.value, request.data.len);
        break;

      case IO_WRITE:
        _write(request.client, request.data.value, request.data.len);
        break;
    }
  }

  return NULL;
}

static inline bool create_io_threads(const uint32_t count) {
  struct IOThreadPool pool = {
    .count = count,
    .threads = malloc(sizeof(pthread_t) * count)
  };

  if (pool.threads == NULL) {
    return false;
  }

  for (uint32_t i = 0; i < count; ++i) {
    pthread_create(&pool.threads[i], NULL, io_worker, NULL);
    pthread_detach(pool.threads[i]);
  }

  if (sem_init(&sem, 0, 0) != 0) {
    return false;
  }

  return true;
}

static inline bool create_io_queue() {
  memory = malloc(sizeof(struct IORequest) * IO_QUEUE_CHUNK);

  if (memory == NULL) {
    return false;
  }

  atomic_store(&at, 0);
  atomic_store(&end, 0);
  return true;
}

bool initialize_io(const uint32_t count) {
  return (create_io_queue() && create_io_threads(count));
}

bool enqueue_io_request(const enum IORequestType type, string_t data, struct Client *client) {
  struct IORequest request = {
    .type = type,
    .data = data,
    .client = client
  };

  memory[atomic_fetch_add_explicit(&end, 1, memory_order_relaxed)] = request;
  sem_post(&sem);

  return true;
}
