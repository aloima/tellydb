#include <telly.h>
#include "io.h"

#include <stdlib.h>

#include <semaphore.h>
#include <sched.h>

void add_io_request(const enum IOOpType type, Client *client, string_t write_str) {
  IOOperation op = {type, client, write_str};

  sem_wait(io_available_space_sem);
  while (!push_tqueue(io_queue, &op)) cpu_relax();

  sem_post(io_stored_sem);
}

static inline bool acquire_client_passive(Client *client) {
  _Atomic(enum ClientState) *state = &client->state;

  enum ClientState expected = CLIENT_STATE_ACTIVE;
  int spin_count = 0;

  while (!ATOMIC_CAS_WEAK(state, &expected, CLIENT_STATE_PASSIVE, memory_order_acq_rel, memory_order_relaxed)) {
    if (expected == CLIENT_STATE_EMPTY) return false;

    while (expected == CLIENT_STATE_PASSIVE) {
      cpu_relax();

      if (++spin_count > 1000) {
        sched_yield();
        spin_count = 0;
      }

      expected = atomic_load_explicit(state, memory_order_relaxed);
      if (expected == CLIENT_STATE_EMPTY) return false;
    }

    expected = CLIENT_STATE_ACTIVE;
  }

  return true;
}

void *handle_io_requests(void *arg) {
  IOThread *thread = arg;

  while (atomic_load_explicit(&thread->status, memory_order_relaxed) == ACTIVE) {
    sem_wait(io_stored_sem);
    IOOperation op;

    if (!pop_tqueue(io_queue, &op)) continue;
    sem_post(io_available_space_sem);

    Client *client = op.client;
    __builtin_prefetch(client, 0, 1);

    if (!acquire_client_passive(client)) continue;

    switch (op.type) {
      case IOOP_GET_COMMAND:
        read_command(thread, client);
        atomic_store_explicit(&client->state, CLIENT_STATE_ACTIVE, memory_order_release);
        break;

      case IOOP_TERMINATE:
        terminate_connection(client);
        break;

      case IOOP_WRITE: {
        string_t *write_str = &op.write_str;
        _write(client, op.write_str.value, op.write_str.len);
        atomic_store_explicit(&client->state, CLIENT_STATE_ACTIVE, memory_order_release);
        break;
      }
    }
  }

  if (thread->read_buf) free(thread->read_buf);
  if (thread->arena) arena_destroy(thread->arena);

  atomic_store_explicit(&thread->status, KILLED, memory_order_release);
  sem_post(io_kill_sem);
  return NULL;
}
