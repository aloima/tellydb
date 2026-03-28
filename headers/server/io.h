#pragma once

#include "server/client.h"
#include "utils/string.h"

#include <stdint.h>

typedef struct {
  pthread_t thread;
  ThreadQueue *queue;
  event_notifier_t *notifier;
  atomic_bool destroyed;

  // Read buffers
  char *buf;
  Arena *resp_arena;
  Arena *ucmd_arena;
} IOThread;

enum IOOpType : uint8_t {
  IOOP_GET_COMMAND,
  IOOP_TERMINATE,
  IOOP_WRITE
};

int create_io_threads();
void send_destroy_signal_to_io_threads();
void add_io_request(const enum IOOpType type, Client *client, string_t write_str);
