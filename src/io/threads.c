#include <telly.h>
#include "io.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>

#include <semaphore.h>
#include <pthread.h>

struct Command *commands = NULL;
IOThread *threads = NULL;
ThreadQueue *queue = NULL;
uint32_t thread_count = 0;

sem_t *kill_sem = NULL;
sem_t *stored_sem = NULL;
sem_t *available_space_sem = NULL;

int create_io_threads(const uint32_t count) {
  bool success = false;
  commands = get_commands();

  kill_sem = malloc(sizeof(sem_t));
  if (kill_sem == NULL || sem_init(kill_sem, 0, 0) != 0) goto CLEANUP;

  stored_sem = malloc(sizeof(sem_t));
  if (stored_sem == NULL || sem_init(stored_sem, 0, 0) != 0) goto CLEANUP;

  available_space_sem = malloc(sizeof(sem_t));
  if (available_space_sem == NULL || sem_init(available_space_sem, 0, 128) != 0) goto CLEANUP;

  queue = create_tqueue(128, sizeof(IOThread), alignof(IOThread));
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
    if (threads) {
      for (uint32_t i = 0; i < count; ++i) {
        IOThread *thread = &threads[i];
        atomic_store_explicit(&thread->status, PASSIVE, memory_order_release);

        if (thread->read_buf) free(thread->read_buf);
        if (thread->arena) arena_destroy(thread->arena);
      }
    }

    if (queue) {
      free_tqueue(queue);
      queue = NULL;
    }

    if (threads) {
      free(threads);
      threads = NULL;
    }

    if (stored_sem) {
      for (uint32_t i = 0; i < count; ++i) sem_post(stored_sem);
      usleep(5);
      sem_destroy(stored_sem);
      free(stored_sem);
    }

    if (kill_sem) {
      sem_destroy(kill_sem);
      free(kill_sem);
    }

    if (available_space_sem) {
      sem_destroy(available_space_sem);
      free(available_space_sem);
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
  }

  for (uint32_t i = 0; i < thread_count; ++i) {
    sem_post(stored_sem);
  }

  for (uint32_t i = 0; i < thread_count; ++i) {
    sem_wait(kill_sem); // wait until killed
  }

  sem_destroy(kill_sem);
  free(kill_sem);

  sem_destroy(stored_sem);
  free(stored_sem);

  sem_destroy(available_space_sem);
  free(available_space_sem);

  free_tqueue(queue);
  free(threads);
}
