#pragma once

#include <telly.h>

#include <stdint.h>
#include <stdatomic.h>

#include <pthread.h>

enum IOThreadStatus : uint8_t {
  ACTIVE,
  PASSIVE,
  KILLED
};

typedef struct {
  enum IOOpType type;
  Client *client;
  string_t write_str;
} IOOperation;

typedef struct {
  uint32_t id;
  pthread_t thread;
  _Atomic enum IOThreadStatus status;

  Arena *arena;
  char *read_buf; // may be initialized as stack if it is not transferred by threads
} IOThread;

void read_command(IOThread *thread, Client *client);
void *handle_io_requests(void *arg);

extern struct Command *commands;
extern struct ThreadQueue *queue;

extern sem_t *kill_sem;
extern sem_t *stored_sem;
extern sem_t *available_space_sem;
