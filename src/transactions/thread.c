#include <telly.h>
#include "transactions.h"

#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <errno.h> // IWYU pragma: export
#include <time.h>

#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>

static pthread_t thread;

static sem_t *thread_sem; // Controls initalization of thread, signalled if thread initialized successfully or not
static _Atomic(bool) kill_pending; // Executes killing operation of thread
static sem_t kill_sem; // Waits until killing thread after kill_pending signal
static int added = -1, efd = -1;

#define THROW_ERROR(error)     \
  write_log(LOG_ERR, (error)); \
  return -1;


static inline int initialize_thread_variables() {
  efd = CREATE_EVENTFD();
  if (efd == -1) {
    THROW_ERROR("Cannot create transaction event waiter, out of memory.");
  }

  tx_notifier = create_notifier();
  if (tx_notifier == NULL) {
    write_log(LOG_ERR, "Cannot allocate transaction notifier, out of memory.");
    return -1;
  }

  const int fd = get_notifier(tx_notifier);

  event_t event;
  CREATE_EVENT(event, fd);

  added = ADD_EVENT(efd, fd, event);
  if (added == -1) {
    write_log(LOG_ERR, "Cannot allocate transaction notifier, out of memory.");
    return -1;
  }

  tx_queue = create_tqueue(server->conf->max_transaction_blocks, sizeof(TransactionBlock *), alignof(TransactionBlock *));
  if (tx_queue == NULL) {
    destroy_notifier(tx_notifier);
    write_log(LOG_ERR, "Cannot allocate transaction blocks, out of memory.");
    return -1;
  }

  thread_sem = sem_open("/transaction_thread", O_CREAT, 0644, 0);
  if (thread_sem == SEM_FAILED) {
    destroy_notifier(tx_notifier);
    free_tqueue(tx_queue);
    write_log(LOG_ERR, "Cannot allocate transaction semaphore, out of memory.");
    return -1;
  }

  sem_init(&kill_sem, 0, 0);
  atomic_init(&kill_pending, false);
  tx_last_saved_at = time(NULL);

  return 0;
}

void *transaction_thread(void *arg) {
  sigset_t *set = (sigset_t *) arg;
  pthread_sigmask(SIG_BLOCK, set, NULL);

  event_t events[1];
  sem_post(thread_sem);

  while (!atomic_load_explicit(&kill_pending, memory_order_relaxed)) {
    WAIT_EVENTS(efd, events, 1, -1);

    TransactionBlock *block;
    uint64_t count = consume_notifier(tx_notifier);

    while (count-- && pop_tqueue(tx_queue, &block)) { // TODO: unknown valgrind warning
      execute_transaction_block(block);
      remove_transaction_block(block);
    }
  }

  destroy_notifier(tx_notifier);
  sem_post(&kill_sem);
  return NULL;
}

void destroy_transaction_thread() {
  atomic_store_explicit(&kill_pending, true, memory_order_relaxed);
  signal_notifier(tx_notifier, 1); // run transaction loop once

  sem_wait(&kill_sem);
  sem_destroy(&kill_sem);
}

int create_transaction_thread() {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);

  if (initialize_thread_variables() == -1) return -1;
  const int code = pthread_create(&thread, NULL, transaction_thread, &set);

  switch (code) {
    case EAGAIN:
      write_log(LOG_ERR, "Cannot create transaction thread, out of memory or thread limit problem of OS.");
      return -1;

    default:
      break;
  }

  pthread_detach(thread); // Thread is guaranteed joinable and available, so no need to control.

  sem_wait(thread_sem);
  sem_close(thread_sem);
  sem_unlink("/transaction_thread");

  return 0;
}
