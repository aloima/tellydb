#pragma once

#include "../utils.h"

#include <stdint.h>
#include <stdbool.h>

#include <pthread.h>

#define IO_QUEUE_CHUNK 8192

struct IOThreadPool {
  pthread_t *threads;
  uint32_t count;
};

enum IORequestType {
  IO_READ,
  IO_WRITE
};

struct IORequest {
  enum IORequestType type;
  string_t data;
  struct Client *client;
};

bool initialize_io(const uint32_t count);
void destroy_io();
bool enqueue_io_request(const enum IORequestType type, string_t data, struct Client *client);
