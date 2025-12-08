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

typedef struct {
  enum IOOpType type;
  Client *client;
  string_t write_str;
} IOOperation;

typedef struct {
  uint32_t id;
  pthread_t thread;
  _Atomic enum IOThreadStatus status;

  Arena *arena;
  char *read_buf; // may be initialized as stack if it is not transferred by threads
} IOThread;

static struct Command *commands;
static IOThread *threads = NULL;
static struct ThreadQueue *queue = NULL;
static uint32_t thread_count = 0;

static sem_t *sem = NULL;

static inline void unknown_command(Client *client, string_t *name) {
  char buf[COMMAND_NAME_MAX_LENGTH + 22];
  const size_t nbytes = sprintf(buf, "-Unknown command '%s'\r\n", name->value);

  _write(client, buf, nbytes);
}

static void read_command(IOThread *thread, Client *client) {
  char *buf = thread->read_buf;
  int32_t size = _read(client, buf, RESP_BUF_SIZE);

  if (size == 0) {
    add_io_request(IOOP_TERMINATE, client, EMPTY_STRING());
    return;
  }

  Arena *arena = thread->arena;
  int32_t at = 0;

  while (size != -1) {
    commanddata_t data;
    if (!get_command_data(arena, client, buf, &at, &size, &data)) continue;

    if (size == at) {
      if (size != RESP_BUF_SIZE) {
        size = -1;
      } else {
        size = _read(client, buf, RESP_BUF_SIZE);
        at = 0;
      }
    }

    if (client->locked) {
      WRITE_ERROR_MESSAGE(client, "Your client is locked, you cannot use any commands until your client is unlocked");
      continue;
    }

    const struct CommandIndex *command_index = get_command_index(data.name->value, data.name->len);

    if (!command_index) {
      unknown_command(client, data.name);
      return;
    }

    const uint64_t command_idx = command_index->idx;
    // client->command = &commands[command_idx];

    if (!add_transaction(client, command_idx, &data)) {
      WRITE_ERROR_MESSAGE(client, "Transaction cannot be enqueued because of server settings");
      write_log(LOG_WARN, "Transaction count reached their limit, so next transactions cannot be added.");
      return;
    }

    if (client->waiting_block && !IS_RELATED_TO_WAITING_TX(commands, command_idx)) _write(client, "+QUEUED\r\n", 9);
  }
}

void *handle_io_requests(void *arg) {
  IOThread *thread = arg;

  while (atomic_load_explicit(&thread->status, memory_order_acquire) == ACTIVE) {
    IOOperation op;

    sem_wait(sem);
    if (!pop_tqueue(queue, &op)) continue;
    thread = arg;

    Client *client = op.client;
    enum ClientState expected = CLIENT_STATE_ACTIVE;

    while (!ATOMIC_CAS_WEAK(&client->state, &expected, CLIENT_STATE_PASSIVE, memory_order_acq_rel, memory_order_relaxed)) {
      if (expected == CLIENT_STATE_EMPTY) goto TERMINATION;
      expected = CLIENT_STATE_ACTIVE;
    }

    switch (op.type) {
      case IOOP_GET_COMMAND:
        read_command(thread, client);
        break;

      case IOOP_TERMINATE:
        terminate_connection(client);
        goto TERMINATION;
        break;

      case IOOP_WRITE: {
        string_t *write_str = &op.write_str;
        _write(op.client, op.write_str.value, op.write_str.len);
        break;
      }

      default:
        break;
    }

    atomic_store_explicit(&client->state, CLIENT_STATE_ACTIVE, memory_order_release);

TERMINATION:
    (void) NULL;
  }

  if (thread->read_buf) free(thread->read_buf);
  if (thread->arena) arena_destroy(thread->arena);

  atomic_store_explicit(&thread->status, KILLED, memory_order_release);
  return NULL;
}

void add_io_request(const enum IOOpType type, Client *client, string_t write_str) {
  IOOperation op = {type, client, write_str};

  // TODO: better waiting
  while (!push_tqueue(queue, &op)) usleep(5);
  sem_post(sem);
}

int create_io_threads(const uint32_t count) {
  bool success = false;
  commands = get_commands();

  sem = malloc(sizeof(sem_t));
  if (sem == NULL || sem_init(sem, 0, 0) != 0) goto CLEANUP;

  queue = create_tqueue(128, sizeof(IOThread), _Alignof(IOThread));
  if (queue == NULL) goto CLEANUP;

  threads = calloc(count, sizeof(IOThread));
  if (threads == NULL) goto CLEANUP;

  thread_count = count;

  for (uint32_t i = 0; i < count; ++i) {
    IOThread *thread = &threads[i];
    thread->id = i;
    atomic_init(&thread->status, ACTIVE);

    thread->arena = arena_create(INITIAL_RESP_ARENA_SIZE);
    if (thread->arena == NULL) goto CLEANUP;

    thread->read_buf = malloc(RESP_BUF_SIZE);
    if (thread->read_buf == NULL) goto CLEANUP;

    int created = pthread_create(&thread->thread, NULL, handle_io_requests, &threads[i]);
    if (created != 0) goto CLEANUP;

    int detached = pthread_detach(thread->thread);
    if (detached != 0) goto CLEANUP;
  }

  success = true;

CLEANUP:
  if (!success) {
    for (uint32_t i = 0; i < count; ++i) {
      IOThread *thread = &threads[i];
      atomic_store_explicit(&thread->status, PASSIVE, memory_order_release);

      if (thread->read_buf) free(thread->read_buf);
      if (thread->arena) arena_destroy(thread->arena);
    }

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

  return (success ? 0 : -1);
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

  uint32_t i = 0;

  while (i < thread_count) {
    if (atomic_load_explicit(&threads[i].status, memory_order_acquire) != KILLED) {
      usleep(10);
      continue;
    }

    i += 1;
  }

  free_tqueue(queue);
  free(threads);
}
