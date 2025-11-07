#pragma once

#include <stddef.h>
#include <stdatomic.h>

enum ThreadQueueState : uint8_t {
  TQ_EMPTY,
  TQ_STORED
};

struct ThreadQueue {
  _Atomic uint64_t at;
  _Atomic uint64_t end;

  _Atomic enum ThreadQueueState *states;
  void *data;
  uint64_t capacity;
  uint64_t type;
};

struct ThreadQueue *create_tqueue(const uint64_t capacity, const uint64_t size, const uint64_t align);
void free_tqueue(struct ThreadQueue *queue);

uint64_t calculate_tqueue_size(const struct ThreadQueue *queue);
void *push_tqueue(struct ThreadQueue *queue, void *value);
void *pop_tqueue(struct ThreadQueue *queue);
void *get_tqueue_value(struct ThreadQueue *queue, const uint64_t idx);
