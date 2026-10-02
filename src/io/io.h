#pragma once

#include <telly.h>

#define IO_QUEUE_SIZE 512

typedef struct {
  IOOpType type;
  Client *client;
  string_t to_write;
} IOOperation;

extern sigset_t io_sigset;

void cleanup_thread(IOThread *thread);
void destroy_thread(IOThread *thread, int added, int efd, int fd);
void *io_thread_procedure(void *arg);