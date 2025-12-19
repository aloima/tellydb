#include <telly.h>

#include <stdlib.h>
#include <stdint.h>

static inline ArenaBlock *arena_block_create(const uint64_t size) {
  ArenaBlock *block = malloc(sizeof(ArenaBlock));
  if (block == NULL) return NULL;

  block->region = malloc(size);
  if (block->region == NULL) {
    free(block);
    return NULL;
  }

  block->index = 0;
  block->size = size;
  block->next = NULL;

  return block;
}

Arena *arena_create(const uint64_t size) {
  Arena *arena = malloc(sizeof(Arena));
  if (arena == NULL) return NULL;

  arena->current = arena_block_create(size);
  if (arena->current == NULL) {
    free(arena);
    return NULL;
  }

  arena->start = arena->current;
  return arena;
}

void arena_destroy(Arena *arena) {
  ArenaBlock *block = arena->start;

  while (block != NULL) {
    ArenaBlock *current = block;
    block = block->next;

    free(current->region);
    free(current);
  }

  free(arena);
}

void *arena_alloc(Arena *arena, const uint64_t size) {
  return arena_alloc_aligned(arena, size, ARENA_DEFAULT_ALIGNMENT);
}

void *arena_alloc_aligned(Arena *arena, const uint64_t size, const uint64_t alignment) {
  ArenaBlock *block = arena->current;

  uint64_t offset = (uint64_t) (block->region + block->index) % alignment;
  if (offset > 0) block->index = block->index - offset + alignment;

  if ((size + (block->index + offset)) > block->size) {
    ArenaBlock *nblock = arena_block_create(block->size);
    if (nblock == NULL) return NULL;

    block->next = nblock;
    arena->current = nblock;
    block = nblock;
  }

  block->index += size;
  return (block->region + (block->index - size));
}
