#include <telly.h>
#include "transactions.h"

static pthread_t thread;

#define SEM_CLOSE(sem, name) do {  \
  ASSERT(sem_close(sem), ==, 0);   \
  ASSERT(sem_unlink(name), ==, 0); \
} while (0)

#define THREAD_SEM "/transaction_thread_sem"
static sem_t *thread_sem = NULL;    // Controls initalization of thread, signalled if thread initialized successfully or not

#define KILL_SEM "/transaction_kill_sem"
static sem_t *kill_sem = NULL;      // Waits until killing thread after kill_pending signal

static _Atomic(bool) kill_pending;  // Executes killing operation of thread
static int added = -1, efd = -1;

#define THROW_ERROR(error) do { \
  write_log(LOG_ERR, (error));  \
  return -1;                    \
} while (0)


static inline int initialize_thread_variables() {
  efd = CREATE_EVENTFD();
  if (efd == -1)
    THROW_ERROR("Cannot create transaction event waiter, out of memory.");

  tx_notifier = create_notifier();
  if (tx_notifier == NULL)
    THROW_ERROR("Cannot allocate transaction notifier, out of memory.");

  const int fd = get_notifier(tx_notifier);

  event_t event;
  CREATE_EVENT(event, fd);

  added = ADD_EVENT(efd, fd, event);
  if (added == -1)
    THROW_ERROR("Cannot allocate transaction notifier, out of memory.");

  tx_queue = create_tqueue(server->conf->max_transaction_blocks, sizeof(TransactionBlock *), alignof(TransactionBlock *));
  if (tx_queue == NULL) {
    destroy_notifier(tx_notifier);
    THROW_ERROR("Cannot allocate transaction blocks, out of memory.");
  }

  thread_sem = sem_open(THREAD_SEM, O_CREAT, 0644, 0);
  if (thread_sem == SEM_FAILED) {
    destroy_notifier(tx_notifier);
    free_tqueue(tx_queue);
    THROW_ERROR("Cannot allocate transaction semaphore, out of memory.");
  }

  kill_sem = sem_open(KILL_SEM, O_CREAT, 0644, 0);
  if (kill_sem == SEM_FAILED) {
    destroy_notifier(tx_notifier);
    free_tqueue(tx_queue);
    SEM_CLOSE(thread_sem, THREAD_SEM);
    THROW_ERROR("Cannot allocate transaction semaphore, out of memory.");
  }

  atomic_init(&kill_pending, false);
  tx_last_saved_at = time(NULL);
  ASSERT(tx_last_saved_at, !=, INVALID_TIME);

  return 0;
}

void *transaction_thread(void *arg) {
  sigset_t *set = (sigset_t *) arg;
  ASSERT(pthread_sigmask(SIG_BLOCK, set, NULL), ==, 0);

  event_t events[1];
  ASSERT(sem_post(thread_sem), ==, 0);

  while (!atomic_load_explicit(&kill_pending, memory_order_relaxed)) {
    TEMP_FAILURE_RETRY(WAIT_EVENTS(efd, events, 1, -1));

    TransactionBlock *block;
    uint64_t count = consume_notifier(tx_notifier);

    // possible TODO: unknown valgrind warning
    while (count > 0) {
      if (atomic_load_explicit(&kill_pending, memory_order_relaxed)) {
        // If there is no unfinished transaction, continue checking for kill_pending thread
        if (!pop_tqueue(tx_queue, &block)) {
          count--;
          continue;
        }
      } else {
        while (!pop_tqueue(tx_queue, &block))
          cpu_relax();
      }

      execute_transaction_block(block);
      remove_transaction_block(block);
      count--;
    }
  }

  destroy_notifier(tx_notifier);
  ASSERT(sem_post(kill_sem), ==, 0);
  return NULL;
}

void destroy_transaction_thread() {
  atomic_store_explicit(&kill_pending, true, memory_order_relaxed);
  signal_notifier(tx_notifier, 1); // run transaction loop once

  ASSERT(sem_wait(kill_sem), ==, 0);
  SEM_CLOSE(kill_sem, KILL_SEM);
}

int create_transaction_thread() {
  sigset_t set;
  ASSERT(sigemptyset(&set), ==, 0);
  ASSERT(sigaddset(&set, SIGINT), ==, 0);
  ASSERT(sigaddset(&set, SIGTERM), ==, 0);

  if (initialize_thread_variables() == -1)
    return -1;

  const int code = pthread_create(&thread, NULL, transaction_thread, &set);

  switch (code) {
    case EAGAIN:
      write_log(LOG_ERR, "Cannot create transaction thread, out of memory or thread limit problem of OS.");
      return -1;

    default:
      break;
  }

  // Thread is guaranteed joinable and available, so no need to control. Assertion is enough.
  ASSERT(pthread_detach(thread), ==, 0);

  sem_wait(thread_sem);
  SEM_CLOSE(thread_sem, THREAD_SEM);

  return 0;
}
