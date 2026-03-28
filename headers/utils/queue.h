#pragma once

#include <stdint.h>

typedef struct Queue {
  uint64_t at;
  uint64_t end;

  void **slots;
  uint64_t capacity, size;
  uint64_t type;
} Queue;

Queue *create_queue(const uint64_t capacity, const uint64_t size, const uint64_t align);
void free_queue(Queue *queue);
void reset_queue(Queue *queue);

void *push_queue(Queue *queue, void *value);
void *pop_queue(Queue *queue);
