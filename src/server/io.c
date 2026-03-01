#include <telly.h>

#include <stdlib.h>
#include <stdatomic.h>
#include <signal.h>
#include <errno.h> // IWYU pragma: export

#include <pthread.h>
#include <unistd.h>

static ThreadQueue *queue = NULL;
static event_notifier_t *notifier = NULL;
static atomic_bool destroyed = false;

typedef struct {
  enum IOOpType type;
  Client *client;
  string_t to_write;
} IOOperation;

void *io_thread(void *arg);

int create_io_thread() {
  sigset_t *set = malloc(sizeof(sigset_t));
  sigemptyset(set);
  sigaddset(set, SIGINT);
  sigaddset(set, SIGTERM);

  notifier = create_notifier();
  if (notifier == NULL) return -1;

  queue = create_tqueue(512, sizeof(IOOperation), alignof(IOOperation));
  if (queue == NULL) {
    destroy_notifier(notifier);
    return -1;
  }

  pthread_t thread;
  const int code = pthread_create(&thread, NULL, io_thread, set);

  switch (code) {
    case EAGAIN:
      destroy_notifier(notifier);
      free_tqueue(queue);
      write_log(LOG_ERR, "Cannot create transaction thread, out of memory or thread limit problem of OS.");
      return -1;

    default:
      break;
  }

  pthread_detach(thread);
  return 0;
}

void destroy_io_thread() {
  atomic_store_explicit(&destroyed, true, memory_order_relaxed);
  signal_notifier(notifier, 1);
}

void add_io_request(const enum IOOpType type, Client *client, string_t to_write) {
  IOOperation op;
  op.type = type;
  op.client = client;

  /**
   * TODO
   * transactions are handled by transaction loop/thread, so IOOP_WRITE requests added by this loop.
   * this loop outs responses to client.write_buf generally, then saves this write_buf to to_write string.
   * After that, this method will be executed. There is a problem, if two transactions from same client will be handled:
   *
   * tx 1 => writes "A" to write_buf, then adds it to I/O requests
   * tx 2 => writes "B" to write_buf, then adds it to I/O requests
   *
   * but, write_buf is only one buffer, so I/O requests from tx 1 and tx 2 consists of "B", not "A"
   * if handling I/O requests is started, there will be collision.
   */
  op.to_write = RESP_OK_MESSAGE("PONG");

  push_tqueue(queue, &op);
  signal_notifier(notifier, 1);
}

void *io_thread(void *arg) {
  const sigset_t *set = (sigset_t *) arg;
  pthread_sigmask(SIG_BLOCK, set, NULL);
  free(arg);

  int added = -1, efd = -1;

  efd = CREATE_EVENTFD();
  if (efd == -1) goto DESTROY;

  const int fd = get_notifier(notifier);

  event_t event;
  CREATE_EVENT(event, fd);

  added = ADD_EVENT(efd, fd, event);
  if (added == -1) goto DESTROY;

  if (initialize_read_buffers() == -1) goto DESTROY;

  // There is exactly one fd/notifier
  event_t events[1];

  while (true) {
    WAIT_EVENTS(efd, events, 1, -1);
    if (atomic_load_explicit(&destroyed, memory_order_relaxed)) break;

    const uint64_t count = consume_notifier(notifier);

    for (uint64_t i = 0; i < count; ++i) {
      IOOperation op;
      if (!pop_tqueue(queue, &op)) break;

      Client *client = op.client;
      if (client->id == -1) continue;

      switch (op.type) {
        case IOOP_TERMINATE:
          terminate_connection(client);
          break;

        case IOOP_GET_COMMAND:
          read_command(client);
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
  destroy_notifier(notifier);
  free_tqueue(queue);
  free_read_buffers();

  return NULL;
}
