#include <telly.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>

#include <semaphore.h>
#include <pthread.h>

enum IOThreadStatus : uint8_t {
  ACTIVE,
  PASSIVE,
  KILLED
};

struct IOOperation {
  enum IOOpType type;
  struct Client *client;
};

struct IOThread {
  uint32_t id;
  pthread_t thread;
  _Atomic enum IOThreadStatus status;
};

static struct IOThread *threads = NULL;
static struct ThreadQueue *queue = NULL;
static uint32_t thread_count = 0;

static sem_t *sem = NULL;

static inline void unknown_command(struct Client *client, commandname_t name) {
  char buf[COMMAND_NAME_MAX_LENGTH + 22];
  const size_t nbytes = sprintf(buf, "-Unknown command '%s'\r\n", name.value);

  _write(client, buf, nbytes);
}

static void read_command(struct Client *client) {
  int32_t at = 0;
  int32_t size = _read(client, client->read_buf, RESP_BUF_SIZE);

  if (size == 0) {
    add_io_request(IOOP_TERMINATE, client);
    return;
  }

  while (size != -1) {
    commanddata_t data;
    if (!get_command_data(client, client->read_buf, &at, &size, &data)) continue;

    if (size == at) {
      if (size != RESP_BUF_SIZE) {
        size = -1;
      } else {
        size = _read(client, client->read_buf, RESP_BUF_SIZE);
        at = 0;
      }
    }

    if (client->locked) {
      free_command_data(&data);
      WRITE_ERROR_MESSAGE(client, "Your client is locked, you cannot use any commands until your client is unlocked");
      continue;
    }

    const struct CommandIndex *command_index = get_command_index(data.name.value, data.name.len);

    if (!command_index) {
      unknown_command(client, data.name);
      return;
    }

    const uint64_t command_idx = command_index->idx;
    // client->command = &commands[command_idx];

    if (!add_transaction(client, command_idx, &data)) {
      free_command_data(&data);
      WRITE_ERROR_MESSAGE(client, "Transaction cannot be enqueued because of server settings");
      write_log(LOG_WARN, "Transaction count reached their limit, so next transactions cannot be added.");
      return;
    }

    if (client->waiting_block && !IS_RELATED_TO_WAITING_TX(command_idx)) _write(client, "+QUEUED\r\n", 9);
  }
}

void *handle_io_requests(void *arg) {
  struct IOThread *thread = ({
    const uint32_t *id = arg;
    &threads[*id];
  });

  while (atomic_load_explicit(&thread->status, memory_order_acquire) == ACTIVE) {
    struct IOOperation op;

    sem_wait(sem);
    if (!pop_tqueue(queue, &op)) continue;

    switch (op.type) {
      case IOOP_GET_COMMAND:
        read_command(op.client);
        break;

      case IOOP_TERMINATE:
        terminate_connection(op.client);
        break;

      default:
        break;
    }
  }

  atomic_store_explicit(&thread->status, KILLED, memory_order_release);
  return NULL;
}

void add_io_request(const enum IOOpType type, struct Client *client) {
  struct IOOperation op = {type, client};
  push_tqueue(queue, &op);
  sem_post(sem);
}

bool create_io_threads(const uint32_t count) {
  bool success = false;

  sem = malloc(sizeof(sem_t));
  if (sem == NULL || sem_init(sem, 0, 0) != 0) goto cleanup;

  queue = create_tqueue(128, sizeof(struct IOThread), _Alignof(struct IOThread));
  if (queue == NULL) goto cleanup;

  threads = malloc(count * sizeof(struct IOThread));
  if (threads == NULL) goto cleanup;

  thread_count = count;

  for (uint32_t i = 0; i < count; ++i) {
    struct IOThread *thread = &threads[i];
    thread->id = i;
    atomic_init(&thread->status, ACTIVE);

    int created = pthread_create(&thread->thread, NULL, handle_io_requests, &thread->id);
    if (created != 0) goto cleanup;

    int detached = pthread_detach(thread->thread);
    if (detached != 0) goto cleanup;
  }


  success = true;

cleanup:
  if (!success) {
    for (uint32_t i = 0; i < count; ++i) atomic_store_explicit(&threads[i].status, PASSIVE, memory_order_release);

    if (queue) {
      free_tqueue(queue);
      queue = NULL;
    }

    if (threads) {
      free(threads);
      threads = NULL;
    }

    if (sem) {
      for (uint32_t i = 0; i < count; ++i) sem_post(sem);
      usleep(5);
      sem_destroy(sem);
      free(sem);
    }
  }

  return success;
}

void destroy_io_threads() {
  if (!threads && queue) {
    free_tqueue(queue);
    return;
  }

  for (uint32_t i = 0; i < thread_count; ++i) {
    atomic_store_explicit(&threads[i].status, PASSIVE, memory_order_release);
    sem_post(sem);
  }

  while (true) {
    bool killed = true;

    for (uint32_t i = 0; i < thread_count; ++i) {
      if (atomic_load_explicit(&threads[i].status, memory_order_acquire) != KILLED) {
        killed = false;
        break;
      }
    }

    if (!killed) {
      usleep(15);
    } else {
      break;
    }
  }

  free_tqueue(queue);
  free(threads);
}
