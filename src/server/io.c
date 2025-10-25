#include <telly.h>

#include <signal.h>
#include <stdbool.h>
#include <stdatomic.h>

#include <pthread.h>
#include <semaphore.h>

static struct IOThreadPool pool;
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
  pool.count = count;
  pool.threads = malloc(sizeof(pthread_t) * count);

  if (pool.threads == NULL) {
    return false;
  }

  for (uint32_t i = 0; i < count; ++i) {
    if (pthread_create(&pool.threads[i], NULL, io_worker, NULL) != 0) {
      for (uint32_t j = 0; j < i; ++j) {
        pthread_kill(pool.threads[j], SIGINT);
      }

      return false;
    }

    if (pthread_detach(pool.threads[i]) != 0) {
      for (uint32_t j = 0; j < i; ++j) {
        pthread_kill(pool.threads[j], SIGINT);
      }

      pthread_kill(pool.threads[i], SIGINT);
      return false;
    }
  }

  if (sem_init(&sem, 0, 0) != 0) {
    for (uint32_t i = 0; i < count; ++i) {
      pthread_kill(pool.threads[i], SIGINT);
    }

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
  if (!create_io_queue()) {
    return false;
  }

  if (!create_io_threads(count)) {
    free(memory);
    return false;
  }

  return true;
}

bool enqueue_io_request(const enum IORequestType type, string_t data, struct Client *client) {
  struct IORequest request = {
    .type = type,
    .data = data,
    .client = client
  };

  memory[atomic_fetch_add_explicit(&end, 1, memory_order_relaxed)] = request;

  if (sem_post(&sem) != 0) {
    atomic_fetch_sub_explicit(&end, 1, memory_order_relaxed);
    return false;
  }

  return true;
}
