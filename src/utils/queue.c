#include <telly.h>

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

struct ThreadQueue *create_tqueue(const uint64_t capacity, const uint64_t size, const uint64_t align) {
  struct ThreadQueue *queue = malloc(sizeof(struct ThreadQueue));
  if (queue == NULL) return NULL;

  void *data;
  if (posix_memalign(&data, align, capacity * size) != 0) {
    free(queue);
    return NULL;
  }

  _Atomic enum ThreadQueueState *states = malloc(sizeof(_Atomic enum ThreadQueueState) * capacity);
  for (uint64_t i = 0; i < capacity; ++i) atomic_init(&states[i], TQ_EMPTY);

  atomic_init(&queue->at, 0);
  atomic_init(&queue->end, 0);

  queue->states = states;
  queue->data = data;

  queue->capacity = capacity;
  queue->type = size;

  return queue;
}

void free_tqueue(struct ThreadQueue *queue) {
  free(queue->states);
  free(queue->data);
  free(queue);
}

uint64_t calculate_tqueue_size(const struct ThreadQueue *queue) {
  const uint64_t current_at = atomic_load_explicit(&queue->at, memory_order_acquire);
  const uint64_t current_end = atomic_load_explicit(&queue->end, memory_order_acquire);

  if (current_end >= current_at) {
    return (current_end - current_at);
  } else {
    return (current_end + (queue->capacity - current_at));
  }
}

void *push_tqueue(struct ThreadQueue *queue, void *value) {
  uint64_t current_end, next_end;

  do {
    const uint64_t current_at = atomic_load_explicit(&queue->at, memory_order_acquire);
    current_end = atomic_load_explicit(&queue->end, memory_order_relaxed);
    next_end = ((current_end + 1) % queue->capacity);

    if (current_at == next_end) return NULL;
  } while (!atomic_compare_exchange_weak_explicit(&queue->end, &current_end, next_end, memory_order_relaxed, memory_order_relaxed));

  char *dst = ((char *) queue->data + (current_end * queue->type));
  memcpy(dst, value, queue->type);
  atomic_store_explicit(&queue->states[current_end], TQ_STORED, memory_order_release);

  return dst;
}

void *pop_tqueue(struct ThreadQueue *queue) {
  uint64_t current_at, next_at, current_end;

  do {
    current_end = atomic_load_explicit(&queue->end, memory_order_relaxed);
    current_at = atomic_load_explicit(&queue->at, memory_order_relaxed);
    if (current_end == current_at) return NULL;

    next_at = ((current_at + 1) % queue->capacity);
  } while (!atomic_compare_exchange_weak_explicit(&queue->at, &current_at, next_at, memory_order_relaxed, memory_order_relaxed));

  char *src = ((char *) queue->data + (current_at * queue->type));
  atomic_store_explicit(&queue->states[current_at], TQ_EMPTY, memory_order_release);

  return src;
}

void *get_tqueue_value(struct ThreadQueue *queue, const uint64_t idx) {
  const uint64_t current_at = atomic_load_explicit(&queue->at, memory_order_acquire);
  const uint64_t current_end = atomic_load_explicit(&queue->end, memory_order_relaxed);

  const uint64_t actual_idx = ((current_at + idx) % queue->capacity);
  if (atomic_load_explicit(&queue->states[actual_idx], memory_order_acquire) != TQ_STORED) return NULL;

  char *src = ((char *) queue->data + (actual_idx * queue->type));
  return src;
}
