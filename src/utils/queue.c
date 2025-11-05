#include <telly.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>

struct ThreadQueue *create_tqueue(const uint64_t capacity, const uint64_t size) {
  struct ThreadQueue *queue = malloc(sizeof(struct ThreadQueue));
  if (queue == NULL) return NULL;

  void *data = malloc(capacity * size);
  if (data == NULL) {
    free(queue);
    return NULL;
  }

  queue->data = data;
  queue->capacity = capacity;
  atomic_init(&queue->at, 0);
  atomic_init(&queue->end, 0);
  queue->type = size;

  return queue;
}

void free_tqueue(struct ThreadQueue *queue) {
  free(queue->data);
  free(queue);
}

static uint64_t calculate_size(const uint64_t at, const uint64_t end, const uint64_t capacity) {
  if (end >= at) {
    return (end - at);
  } else {
    return (end + (capacity - at));
  }
}

uint64_t calculate_tqueue_size(const struct ThreadQueue *queue) {
  const uint64_t current_at = atomic_load_explicit(&queue->at, memory_order_acquire);
  const uint64_t current_end = atomic_load_explicit(&queue->end, memory_order_acquire);

  return calculate_size(current_at, current_end, queue->capacity);
}

bool push_tqueue(struct ThreadQueue *queue, void *value) {
  uint64_t current_end, next_end;

  do {
    const uint64_t current_at = atomic_load_explicit(&queue->at, memory_order_relaxed);
    current_end = atomic_load_explicit(&queue->end, memory_order_relaxed);
    next_end = ((current_end + 1) % queue->capacity);

    if (current_at == next_end) return false;
  } while (!atomic_compare_exchange_weak_explicit(&queue->end, &current_end, next_end, memory_order_release, memory_order_relaxed));

  memcpy((char *) queue->data + (current_end * queue->type), value, queue->type);
  return true;
}

bool pop_tqueue(struct ThreadQueue *queue) {
  uint64_t current_at, next_at;

  do {
    const uint64_t current_end = atomic_load_explicit(&queue->end, memory_order_relaxed);
    current_at = atomic_load_explicit(&queue->at, memory_order_relaxed);
    if (current_end == current_at) return false;

    next_at = ((current_at + 1) % queue->capacity);
  } while (!atomic_compare_exchange_weak_explicit(&queue->at, &current_at, next_at, memory_order_release, memory_order_relaxed));

  return true;
}

void *get_tqueue_value(struct ThreadQueue *queue, const uint64_t idx) {
  const uint64_t current_at = atomic_load_explicit(&queue->at, memory_order_acquire);
  const uint64_t current_end = atomic_load_explicit(&queue->end, memory_order_relaxed);

  const uint64_t size = calculate_size(current_at, current_end, queue->capacity);
  if (idx >= size) return NULL;

  const uint64_t actual_idx = ((current_at + idx) % queue->capacity);

  return ((char *) queue->data + (actual_idx * queue->type));
}
