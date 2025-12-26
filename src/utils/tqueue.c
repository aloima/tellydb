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

static inline bool init_state(struct ThreadQueue *queue, uint64_t at, uint64_t align, uint64_t size) {
  atomic_init(&queue->slots[at].seq, at);

  if (posix_memalign((void **) &queue->slots[at].data, align, size) != 0) {
    for (uint64_t i = 0; i < at; ++i) {
      free(queue->slots[i].data);
    }

    free(queue->slots);
    free(queue);
    return false;
  }

  return true;
}

ThreadQueue *create_tqueue(const uint64_t capacity, const uint64_t size, uint64_t align) {
  // Capacity and alignment control for power of 2
  if ((capacity == 0) || ((capacity & (capacity - 1)) != 0)) return NULL;
  if (align < sizeof(void *)) align = sizeof(void *);
  if ((align & (align - 1)) != 0) return NULL;

  ThreadQueue *queue;
  if (amalloc(queue, ThreadQueue, 1) != 0) {
    return NULL;
  }

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

  atomic_init(&queue->at, 0);
  atomic_init(&queue->end, 0);

  queue->capacity = capacity;
  queue->type = size;

  return queue;
}

void free_tqueue(ThreadQueue *queue) {
  for (uint64_t i = 0; i < queue->capacity; ++i) {
    free(queue->slots[i].data);
  }

  free(queue->slots);
  free(queue);
}

uint64_t estimate_tqueue_size(const ThreadQueue *queue) {
  const uint64_t at = atomic_load_explicit(&queue->at, memory_order_acquire);
  const uint64_t end = atomic_load_explicit(&queue->end, memory_order_acquire);

  return (end - at); // automatically wanted behavior
}

void *push_tqueue(ThreadQueue *queue, void *value) {
  __builtin_prefetch(value, 0, 3);

  _Atomic(uint64_t) *qat = &queue->at;
  _Atomic(uint64_t) *qend = &queue->end;
  const uint64_t capacity = queue->capacity;

  ThreadQueueSlot *slot;
  const uint64_t mask = (capacity - 1);
  uint64_t end = atomic_load_explicit(qend, memory_order_relaxed);

  while (true) {
    slot = &queue->slots[end & mask];
    uint64_t seq = atomic_load_explicit(&slot->seq, memory_order_acquire);
    int64_t diff = seq - end;

    if (diff == 0) {
      if (ATOMIC_CAS_WEAK(qend, &end, end + 1, memory_order_acq_rel, memory_order_relaxed)) {
        break;
      }
    } else if (diff < 0) {
      return NULL;
    } else {
      end = atomic_load_explicit(qend, memory_order_relaxed);
    }

    cpu_relax();
  }

  char *dst = slot->data;
  __builtin_prefetch(dst, 1, 3);
  memcpy(dst, value, queue->type);
  atomic_store_explicit(&slot->seq, end + 1, memory_order_release);

  return dst;
}

bool pop_tqueue(ThreadQueue *queue, void *dst) {
  __builtin_prefetch(dst, 1, 3);

  _Atomic(uint64_t) *qat = &queue->at;
  _Atomic(uint64_t) *qend = &queue->end;

  ThreadQueueSlot *slot;
  const uint64_t mask = (queue->capacity - 1);
  uint64_t at = atomic_load_explicit(qat, memory_order_relaxed);

  while (true) {
    slot = &queue->slots[at & mask];
    uint64_t seq = atomic_load_explicit(&slot->seq, memory_order_acquire);
    int64_t diff = seq - (at + 1);

    if (diff == 0) {
      if (ATOMIC_CAS_WEAK(qat, &at, at + 1, memory_order_acq_rel, memory_order_relaxed)) {
        break;
      }
    } else if (diff < 0) {
      return NULL;
    } else {
      at = atomic_load_explicit(qat, memory_order_relaxed);
    }

    cpu_relax();
  }

  memcpy(dst, slot->data, queue->type);
  atomic_store_explicit(&slot->seq, at + queue->capacity, memory_order_release);
  return true;
}
