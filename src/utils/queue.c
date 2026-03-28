#include <telly.h>

static inline bool init_state(struct Queue *queue, uint64_t at, uint64_t align, uint64_t size) {
  if (posix_memalign((void **) &queue->slots[at], align, size) != 0) {
    for (uint64_t i = 0; i < at; ++i) {
      free(queue->slots[i]);
    }

    free(queue->slots);
    free(queue);
    return false;
  }

  return true;
}

Queue *create_queue(const uint64_t capacity, const uint64_t size, uint64_t align) {
  // Capacity and alignment control for power of 2
  if (capacity == 0) return NULL;
  if (align < sizeof(void *)) align = sizeof(void *);
  if ((align & (align - 1)) != 0) return NULL;

  Queue *queue;
  if (amalloc(queue, Queue, 1) != 0) return NULL;

  if (posix_memalign((void **) &queue->slots, align, capacity * sizeof(ThreadQueueSlot)) != 0) {
    free(queue);
    return NULL;
  }

  for (uint64_t i = 0; i < capacity / 4; ++i) {
    const uint64_t idx = (i * 4);
    if (!init_state(queue, idx, align, size)) return NULL;
    if (!init_state(queue, idx + 1, align, size)) return NULL;
    if (!init_state(queue, idx + 2, align, size)) return NULL;
    if (!init_state(queue, idx + 3, align, size)) return NULL;
  }

  const uint64_t offset = ((capacity / 4) * 4);

  for (uint64_t i = offset; i < capacity; ++i) {
    if (!init_state(queue, i, align, size)) return NULL;
  }

  queue->capacity = capacity;
  queue->type = size;

  return queue;
}

void reset_queue(Queue *queue) {
  queue->size = 0;
  queue->at = 0;
  queue->end = 0;
}

void free_queue(Queue *queue) {
  for (uint64_t i = 0; i < queue->capacity; ++i) {
    free(queue->slots[i]);
  }

  free(queue->slots);
  free(queue);
}

void *push_queue(Queue *queue, void *value) {
  // There is no possibility for prefetching because of time-lacking.
  if (queue->size == queue->capacity) return NULL;

  void *res = queue->slots[queue->end];
  queue->end = ((queue->end + 1) % queue->capacity);

  memcpy(res, value, queue->type);
  queue->size += 1;

  return res;
}

void *pop_queue(Queue *queue) {
  // There is no possibility for prefetching because of time-lacking.
  if (queue->size == 0) return NULL;

  void *res = queue->slots[queue->at];
  queue->at = ((queue->at + 1) % queue->capacity);

  queue->size -= 1;
  return res;
}
