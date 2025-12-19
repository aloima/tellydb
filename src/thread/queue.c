#include <telly.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdalign.h>
#include <stdatomic.h>

#if defined(__x86_64__) || defined(__i386__)
  #define cpu_relax() __asm__ __volatile__("pause\n" ::: "memory")
#elif defined(__aarch64__)
  #define cpu_relax() __asm__ __volatile__("yield\n" ::: "memory")
#endif

struct ThreadQueue *create_tqueue(const uint64_t capacity, const uint64_t size, uint64_t align) {
  // Capacity and alignment control for power of 2
  if ((capacity == 0) || ((capacity & (capacity - 1)) != 0)) return NULL;
  if (align < sizeof(void *)) align = sizeof(void *);
  if ((align & (align - 1)) != 0) return NULL;

  struct ThreadQueue *queue;
  if (posix_memalign((void **) &queue, alignof(typeof(struct ThreadQueue)), sizeof(struct ThreadQueue)) != 0) {
    return NULL;
  }

  void *data;
  if (posix_memalign(&data, align, capacity * size) != 0) {
    free(queue);
    return NULL;
  }

  void *states;
  if (posix_memalign(&states, alignof(typeof(queue->states[0])), capacity * sizeof(queue->states[0])) != 0) {
    free(data);
    free(queue);
    return NULL;
  }

  // It is undefined behavior.
  // memset_aligned(states, 0, capacity * sizeof(queue->states[0]));

  queue->states = states;
  queue->data = data;

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
  const uint64_t at = atomic_load_explicit(&queue->at, memory_order_acquire);
  const uint64_t end = atomic_load_explicit(&queue->end, memory_order_acquire);

  return (end - at); // automatically wanted behavior
}

void *push_tqueue(struct ThreadQueue *queue, void *value) {
  __builtin_prefetch(value, 0, 3);

  _Atomic(uint64_t) *qat = &queue->at;
  _Atomic(uint64_t) *qend = &queue->end;
  const uint64_t capacity = queue->capacity;

  const uint64_t mask = (capacity - 1);
  uint64_t end = atomic_load_explicit(qend, memory_order_relaxed);

  while (true) {
    const uint64_t at = atomic_load_explicit(qat, memory_order_acquire);
    if ((end - at) >= capacity) return NULL;

    if (ATOMIC_CAS_WEAK(qend, &end, end + 1, memory_order_acq_rel, memory_order_relaxed)) {
      break;
    }

    cpu_relax();
  }

  const uint64_t idx = (end & mask);

  _Atomic(enum ThreadQueueState) *state = &queue->states[idx].value;
  __builtin_prefetch(state, 1, 3);

  char *dst = (((char *) queue->data) + (idx * queue->type));
  __builtin_prefetch(dst, 1, 3);

  enum ThreadQueueState expected = TQ_EMPTY;
  while (!ATOMIC_CAS_WEAK(state, &expected, TQ_STORING, memory_order_acq_rel, memory_order_relaxed)) {
    expected = TQ_EMPTY;
    cpu_relax();
  }

  memcpy(dst, value, queue->type);

  atomic_store_explicit(state, TQ_STORED, memory_order_release);
  return dst;
}

bool pop_tqueue(struct ThreadQueue *queue, void *dst) {
  __builtin_prefetch(dst, 1, 3);

  _Atomic(uint64_t) *qat = &queue->at;
  _Atomic(uint64_t) *qend = &queue->end;

  const uint64_t mask = (queue->capacity - 1);
  uint64_t at = atomic_load_explicit(qat, memory_order_relaxed);

  while (true) {
    const uint64_t end = atomic_load_explicit(qend, memory_order_acquire);
    if (end == at) return false;

    if (ATOMIC_CAS_WEAK(qat, &at, at + 1, memory_order_acq_rel, memory_order_relaxed)) {
      break;
    }

    cpu_relax();
  }

  __builtin_prefetch(dst, 1, 0);
  const uint64_t idx = at & mask;

  _Atomic(enum ThreadQueueState) *state = &queue->states[idx].value;
  __builtin_prefetch(state, 1, 3);

  char *src = (((char *) queue->data) + (idx * queue->type));
  __builtin_prefetch(src, 0, 3);

  enum ThreadQueueState expected = TQ_STORED;
  while (!ATOMIC_CAS_WEAK(state, &expected, TQ_CONSUMING, memory_order_acq_rel, memory_order_relaxed)) {
    expected = TQ_STORED;
    cpu_relax();
  }

  memcpy(dst, src, queue->type);

  atomic_store_explicit(state, TQ_EMPTY, memory_order_release);
  return true;
}
