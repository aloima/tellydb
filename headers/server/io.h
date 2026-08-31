#pragma once

#include "client.h"
#include "utils/utils.h"

#include <stdint.h>

#include <pthread.h>

typedef enum : uint8_t {
  IOOP_READ,
  IOOP_WRITE,
  IOOP_TERMINATE
} IOOpType;

typedef enum : uint8_t {
  IO_THREAD_ACTIVE,
  IO_THREAD_PENDING_DESTROY,
  IO_THREAD_DESTROYED
} IOThreadStatus;

typedef struct {
  pthread_t thread;
  ThreadQueue *queue;
  event_notifier_t *notifier; // For catching I/O operations, used inside I/O thread

  _Atomic(IOThreadStatus) status;

  Arena *ucmd_arena;
} IOThread;

IOThread *get_io_threads();
int64_t get_io_thread_count();

int create_io_threads();
void send_destroy_signal_to_io_threads();
int add_io_request(const IOOpType type, Client *client, string_t write_str);
