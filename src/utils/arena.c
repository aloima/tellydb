#include <telly.h>

#include <stdlib.h>
#include <stdint.h>

Arena *arena_create(const uint64_t size) {
  Arena *arena = malloc(sizeof(Arena));
  if (arena == NULL) return NULL;

  arena->region = malloc(size);
  if (arena->region == NULL) {
    free(arena);
    return NULL;
  }

  arena->index = 0;
  arena->size = size;

  return arena;
}

void arena_destroy(Arena *arena) {
  free(arena->region);
  free(arena);
}

void *arena_alloc(Arena *arena, const uint64_t size) {
  return arena_alloc_aligned(arena, size, ARENA_DEFAULT_ALIGNMENT);
}

void *arena_alloc_aligned(Arena *arena, const uint64_t size, const uint64_t alignment) {
  uint64_t offset = (uint64_t) (arena->region + arena->index) % alignment;
  if (offset > 0) {
    arena->index = arena->index - offset + alignment;
  }

  if ((size + (arena->index + offset)) > arena->size) {
    arena->size *= 2;
    arena->region = realloc(arena->region, arena->size);

    if (arena->region == NULL) {
      arena->size /= 2;
      return NULL;
    }
  }

  arena->index += size;
  return (arena->region + (arena->index - size));
}
