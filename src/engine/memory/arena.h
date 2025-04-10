#ifndef __ARENA_ALLOCATOR_H__
#define __ARENA_ALLOCATOR_H__

#include "define.h"

typedef struct arena_allocator_t {
	uint64_t total_size;
	uint64_t prev_offset;
	uint64_t curr_offset;
	void *memory;
	uint8_t *buff;
	b8 own_memory;
} arena_allocator_t;

void arena_init(uint64_t total_size, void *memory,
                      arena_allocator_t *allocator);
void arena_shut(arena_allocator_t *allocator);
void *arena_allocate(arena_allocator_t *allocator, uint64_t size);
void *arena_allocate_align(arena_allocator_t *allocator, uint64_t size,
                           uintptr_t alignment);
void arena_free_all(arena_allocator_t *allocator);

#endif //__ARENA_ALLOCATOR_H__
