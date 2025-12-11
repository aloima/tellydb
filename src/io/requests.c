#include <telly.h>
#include "io.h"

void add_io_request(const enum IOOpType type, Client *client, string_t write_str) {
  IOOperation op = {type, client, write_str};

  sem_wait(available_space_sem);
  while (!push_tqueue(queue, &op));

  sem_post(stored_sem);
}

void *handle_io_requests(void *arg) {
  IOThread *thread = arg;

  while (atomic_load_explicit(&thread->status, memory_order_acquire) == ACTIVE) {
    sem_wait(stored_sem);
    IOOperation op;

    if (!pop_tqueue(queue, &op)) continue;
    sem_post(available_space_sem);
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
  sem_post(kill_sem);
  return NULL;
}
