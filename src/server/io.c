#include <telly.h>

#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h> // IWYU pragma: export

#include <pthread.h>
#include <unistd.h>

static ThreadQueue *queue = NULL;
static event_notifier_t *notifier = NULL;
static bool destroyed = false;

typedef struct {
  enum IOOpType type;
  Client *client;
} IOOperation;

void *io_thread(void *arg);

int create_io_thread() {
  sigset_t *set = malloc(sizeof(sigset_t));
  sigemptyset(set);
  sigaddset(set, SIGINT);
  sigaddset(set, SIGTERM);

  pthread_t thread;
  const int code = pthread_create(&thread, NULL, io_thread, set);

  switch (code) {
    case EAGAIN:
      write_log(LOG_ERR, "Cannot create transaction thread, out of memory or thread limit problem of OS.");
      return -1;

    default:
      break;
  }

  pthread_detach(thread);
  return 0;
}

void destroy_io_thread() {
  destroyed = true;
  usleep(10);

  signal_notifier(notifier);
}

void add_io_request(const enum IOOpType type, Client *client, string_t write_str) {
  IOOperation *op = malloc(sizeof(IOOperation));
  op->type = type;
  op->client = client;

  push_tqueue(queue, &op);
  signal_notifier(notifier);
}

void *io_thread(void *arg) {
  const sigset_t *set = (sigset_t *) arg;
  pthread_sigmask(SIG_BLOCK, set, NULL);
  free(arg);

  int added = -1, efd = -1;

  notifier = create_notifier();
  if (notifier == NULL) goto DESTROY;

  queue = create_tqueue(512, sizeof(IOOperation), alignof(IOOperation));
  if (queue == NULL) goto DESTROY;

  efd = CREATE_EVENTFD();
  if (efd == -1) goto DESTROY;

  const int fd = get_notifier(notifier);

  event_t event;
  CREATE_EVENT(event, fd);

  added = ADD_EVENT(efd, fd, event);
  if (added == -1) goto DESTROY;

  event_t events[256];

  while (true) {
    int n = WAIT_EVENTS(efd, events, 256);
    if (destroyed) break;

    for (int i = 0; i < n; ++i) {
      IOOperation *op;
      pop_tqueue(queue, &op);
      consume_notifier(notifier);

      // TODO: handle I/O operations
      free(op);
    }
  }

DESTROY:
  if (added != -1) {
    event_t ev;
    PREPARE_REMOVING_EVENT(ev, fd);
    REMOVE_EVENT(efd, fd);
  }

  if (efd != -1) close(efd);
  if (notifier != NULL) destroy_notifier(notifier);
  if (queue != NULL) free_tqueue(queue);

  return NULL;
}
