#include <telly.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>

#if defined(__x86_64__) || defined(__i386__)
  #define cpu_relax() __asm__ __volatile__("pause\n" ::: "memory")
#elif defined(__aarch64__)
  #define cpu_relax() __asm__ __volatile__("yield\n" ::: "memory")
#endif

struct ThreadQueue *create_tqueue(const uint64_t capacity, const uint64_t size, const uint64_t align) {
  struct ThreadQueue *queue = malloc(sizeof(struct ThreadQueue));
  if (queue == NULL) return NULL;

  void *data;
  if (posix_memalign(&data, align, capacity * size) != 0) {
    free(queue);
    return NULL;
  }

  void *states;
  if (posix_memalign(&states, 64, capacity * sizeof(queue->states[0])) != 0) {
    free(data);
    free(queue);
    return NULL;
  }

  queue->data = data;
  queue->states = states;

  for (uint64_t i = 0; i < capacity; ++i) {
    atomic_init(&queue->states[i].value, TQ_EMPTY);
  }

  atomic_init(&queue->at, 0);
  atomic_init(&queue->end, 0);

  queue->capacity = capacity;
  queue->type = size;

  return queue;
}

void free_tqueue(struct ThreadQueue *queue) {
  free(queue->states);
  free(queue->data);
  free(queue);
}

uint64_t estimate_tqueue_size(const struct ThreadQueue *queue) {
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
    next_end = ((current_end + 1) & (queue->capacity - 1));

    if (current_at == next_end) return NULL;
  } while (!atomic_compare_exchange_weak_explicit(&queue->end, &current_end, next_end, memory_order_acq_rel, memory_order_relaxed));

  while (atomic_load_explicit(&queue->states[current_end].value, memory_order_acquire) != TQ_EMPTY) {
    cpu_relax();
  }

  atomic_store_explicit(&queue->states[current_end].value, TQ_STORING, memory_order_relaxed);
  char *dst = ((char *) queue->data + (current_end * queue->type));
  memcpy(dst, value, queue->type);
  atomic_store_explicit(&queue->states[current_end].value, TQ_STORED, memory_order_release);

  return dst;
}

bool pop_tqueue(struct ThreadQueue *queue, void *dest) {
  uint64_t current_at, next_at;

  do {
    const uint64_t current_end = atomic_load_explicit(&queue->end, memory_order_acquire);
    current_at = atomic_load_explicit(&queue->at, memory_order_relaxed);
    if (current_end == current_at) return false;

    next_at = ((current_at + 1) & (queue->capacity - 1));
  } while (!atomic_compare_exchange_weak_explicit(&queue->at, &current_at, next_at, memory_order_acq_rel, memory_order_relaxed));

  while (atomic_load_explicit(&queue->states[current_at].value, memory_order_acquire) != TQ_STORED) {
    cpu_relax();
  }

  atomic_store_explicit(&queue->states[current_at].value, TQ_RETRIEVING, memory_order_relaxed);
  char *src = ((char *) queue->data + (current_at * queue->type));
  memcpy(dest, src, queue->type);
  atomic_store_explicit(&queue->states[current_at].value, TQ_EMPTY, memory_order_release);

  return true;
}
