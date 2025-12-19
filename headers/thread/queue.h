#pragma once

#include <stddef.h>
#include <stdatomic.h>
#include <stdalign.h>

enum ThreadQueueState : uint8_t {
  TQ_EMPTY = 0,
  TQ_STORING,
  TQ_CONSUMING,
  TQ_STORED
};

struct ThreadQueueStateValue {
  alignas(64) _Atomic(enum ThreadQueueState) value;
};

struct ThreadQueue {
  struct ThreadQueueStateValue *states;
  void *data;
  uint64_t capacity;
  uint64_t type;

  alignas(64) _Atomic(uint64_t) at;
  alignas(64) _Atomic(uint64_t) end;
};

struct ThreadQueue *create_tqueue(const uint64_t capacity, const uint64_t size, const uint64_t align);
void free_tqueue(struct ThreadQueue *queue);

uint64_t estimate_tqueue_size(const struct ThreadQueue *queue);
void *push_tqueue(struct ThreadQueue *queue, void *value);
bool pop_tqueue(struct ThreadQueue *queue, void *dest);
