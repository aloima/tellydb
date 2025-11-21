#include <telly.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>

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

static struct IOThread *threads;
static struct ThreadQueue *queue;
static uint32_t thread_count = 0;

static inline void unknown_command(struct Client *client, commandname_t name) {
  char *buf = malloc(name.len + 22);
  const size_t nbytes = sprintf(buf, "-Unknown command '%s'\r\n", name.value);

  _write(client, buf, nbytes);
  free(buf);
}

static void read_command(struct Client *client) {
  int32_t at = 0;
  int32_t size = _read(client, client->read_buf, RESP_BUF_SIZE);

  if (size == 0) {
    terminate_connection(client);
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
      free_command_data(data);
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
      free_command_data(data);
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
    struct IOOperation *area = pop_tqueue(queue);

    if (area == NULL) {
      usleep(1);
      continue;
    }

    struct IOOperation op = *area;

    switch (op.type) {
      case IOOP_GET_COMMAND:
        read_command(op.client);
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
}

void create_io_threads(const uint32_t count) {
  queue = create_tqueue(128, sizeof(struct IOThread), _Alignof(struct IOThread));
  threads = malloc(count * sizeof(struct IOThread));
  thread_count = count;

  for (uint32_t i = 0; i < count; ++i) {
    threads[i].id = i;
    atomic_init(&threads[i].status, ACTIVE);
    pthread_create(&threads[i].thread, NULL, handle_io_requests, &threads[i].id);
    pthread_detach(threads[i].thread);
  }

}

void destroy_io_threads() {
  for (uint32_t i = 0; i < thread_count; ++i) {
    atomic_store_explicit(&threads[i].status, PASSIVE, memory_order_release);
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
