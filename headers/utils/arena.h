#pragma once

#include <stdint.h>

#define ARENA_DEFAULT_ALIGNMENT 8

typedef struct ArenaBlock {
  struct ArenaBlock *next;
  uint8_t *region;
  uint64_t index;
  uint64_t size;
} ArenaBlock;

typedef struct {
  ArenaBlock *current;
  ArenaBlock *start;
} Arena;

Arena *arena_create(const uint64_t size);
void arena_destroy(Arena *arena);

void *arena_alloc(Arena *arena, const uint64_t size);
void *arena_alloc_aligned(Arena *arena, const uint64_t size, const uint64_t alignment);
