#pragma once

#include <stddef.h>
#include <stdatomic.h>
#include <stdalign.h>

typedef struct {
  _Atomic uint64_t seq;
  void *data;
  char _pad[64 - sizeof(uint64_t) - sizeof(void *)];
} ThreadQueueSlot;

typedef struct ThreadQueue {
  alignas(64) _Atomic(uint64_t) at;
  alignas(64) _Atomic(uint64_t) end;

  ThreadQueueSlot *slots;
  uint64_t capacity;
  uint64_t type;
} ThreadQueue;

ThreadQueue *create_tqueue(const uint64_t capacity, const uint64_t size, const uint64_t align);
void free_tqueue(struct ThreadQueue *queue);

uint64_t estimate_tqueue_size(const ThreadQueue *queue);
void *push_tqueue(ThreadQueue *queue, void *value);
bool pop_tqueue(ThreadQueue *queue, void *dest);
