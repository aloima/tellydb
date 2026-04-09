#pragma once

#include "client.h"
#include "utils/utils.h"

#include <stdint.h>

#include <pthread.h>

enum IOOpType : uint8_t {
  IOOP_READ,
  IOOP_WRITE,
  IOOP_TERMINATE
};

enum IOThreadStatus {
  IO_THREAD_ACTIVE,
  IO_THREAD_PENDING_DESTROY,
  IO_THREAD_DESTROYED
};

typedef struct {
  pthread_t thread;
  ThreadQueue *queue;
  event_notifier_t *notifier; // For catching I/O operations, used inside I/O thread
  event_notifier_t *server_notifier; // For catching emptiness of I/O operations, used inside server
  _Atomic(enum IOThreadStatus) status;

  // Read buffers
  char *buf;
  Arena *resp_arena;
  Arena *ucmd_arena;
} IOThread;

IOThread *get_io_threads();
int64_t get_io_thread_count();

int create_io_threads();
void send_destroy_signal_to_io_threads();
int add_io_request(const enum IOOpType type, Client *client, string_t write_str);
