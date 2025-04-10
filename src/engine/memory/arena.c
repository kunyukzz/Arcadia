#include "arena.h"
#include "engine/memory/memory.h"
#include "engine/core/logger.h"
#include "engine/core/assertion.h"

#define DEFAULT_ALIGNMENT 0x10 // 16

void arena_init(uint64_t total_size, void *memory, arena_allocator_t *allocator) {
	if (allocator) {
		allocator->total_size = total_size;
		allocator->curr_offset = 0;
		allocator->own_memory = memory == 0;

		if (memory) {
			allocator->memory = memory;
		} else {
			allocator->memory = memory_alloc(total_size, MEMTAG_ARENA_ALLOCATOR);
		}
	}
}

void arena_shut(arena_allocator_t *allocator) {
	if (allocator) {
		allocator->curr_offset = 0;

		if (allocator->own_memory && allocator->memory)
		  	memory_free(allocator->memory, allocator->total_size,
					  	MEMTAG_ARENA_ALLOCATOR);

        allocator->memory = 0;
		allocator->total_size = 0;
		allocator->own_memory = 0;
	}
}

void *arena_allocate(arena_allocator_t *allocator, uint64_t size) {
	return arena_allocate_align(allocator, size, DEFAULT_ALIGNMENT);
}

void *arena_allocate_align(arena_allocator_t *allocator, uint64_t size,
                           uintptr_t alignment) {
	ar_assert_msg((alignment & (alignment - 1)) == 0,
			"Alignment must be a power of 2");
	if (!allocator || !allocator->memory) {
        ar_ERROR("Arena Allocator - provided allocator not allocated");
        return 0;
    }

	uintptr_t curr_addr = (uintptr_t)allocator->memory + allocator->curr_offset;
	uintptr_t align_addr = (curr_addr + (alignment - 1)) & ~(alignment - 1);
	uint64_t padding = align_addr - curr_addr;
	uint64_t total_size = size + padding;

	if (allocator->curr_offset + total_size > allocator->total_size) {
        uint64_t remain = allocator->total_size - allocator->curr_offset;
        ar_ERROR("Arena Allocator tried to allocate %lluB aligned to %luB, "
                 "only %lluB remains",
                 size, alignment, remain);
        return 0;
    }

	allocator->prev_offset = allocator->curr_offset;
	allocator->curr_offset += total_size;

	return (void *)align_addr;
}

void arena_free_all(arena_allocator_t *allocator) {
	if (allocator && allocator->memory) {
		allocator->curr_offset = 0;
		memory_zero(allocator->memory, allocator->total_size);
	}
}

