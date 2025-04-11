#include "engine/memory/stack.h"
#include "engine/memory/memory.h"
#include "engine/core/logger.h"
#include "engine/core/assertion.h"

#define DEFAULT_ALIGNMENT 0x10 // 16

stack_marker stack_get_marker(stack_allocator_t *allocator) {
	ar_assert(allocator != 0);
	ar_assert(allocator->memory != 0);
	ar_assert(allocator->offset <= allocator->total_size);

	return allocator->offset;
}

void stack_free_to_marker(stack_allocator_t *allocator, stack_marker marker) {
	ar_assert(allocator != 0);
	ar_assert(allocator->memory != 0);
	ar_assert(marker <= allocator->offset);
	ar_assert(marker <= allocator->total_size);

	allocator->offset = marker;
}

void stack_init(uint64_t total_size, void *memory, stack_allocator_t *allocator) {
	if (allocator) {
		allocator->total_size = total_size;
		allocator->offset = 0;
		allocator->own_memory = memory == 0;

		if (memory) {
			allocator->memory = memory;
		} else {
			allocator->memory = memory_alloc(total_size, MEMTAG_STACK_ALLOCATOR);
			ar_assert_msg(allocator->memory, "stack_init: failed to allocate memory");
		}
	}
}

void stack_shut(stack_allocator_t *allocator) {
	if (allocator) {
		allocator->offset = 0;

		if (allocator->own_memory && allocator->memory)
		  	memory_free(allocator->memory, allocator->total_size,
					  	MEMTAG_ARENA_ALLOCATOR);

        allocator->memory = 0;
		allocator->total_size = 0;
		allocator->own_memory = 0;
	}
}

void *stack_allocate(stack_allocator_t *allocator, uint64_t size) {
	return stack_allocate_align(allocator, size, DEFAULT_ALIGNMENT);
}

void *stack_allocate_align(stack_allocator_t *allocator, uint64_t size,
		uintptr_t alignment) {
	ar_assert_msg((alignment & (alignment - 1)) == 0,
			"Alignment must be a power of 2");	
	if (!allocator || !allocator->memory) {
		ar_ERROR("Stack Allocator - provided allocator not allocated");
		return 0;
	}

	uintptr_t raw_ptr = (uintptr_t)allocator->memory + allocator->offset;
	uintptr_t align_ptr = (raw_ptr + (alignment - 1 )) & ~(alignment - 1);
	uint64_t padding = align_ptr - raw_ptr;
	uint64_t total_size = size + padding;

	if (allocator->offset + total_size > allocator->total_size) {
		uint64_t remain = allocator->total_size - allocator->offset;
		ar_WARNING("Stack Allocator tried to allocate %lluB aligned to "
				   "%luB, only %lluB remains", size, alignment, remain);
			return 0;
        }
	
	void *result = (void *)align_ptr;
	allocator->offset += total_size;
	return result;
}

void stack_reset(stack_allocator_t *allocator) {
	if (allocator && allocator->memory)
		allocator->offset = 0;
}

void stack_hard_reset(stack_allocator_t *allocator) {
	if (allocator && allocator->memory) {
		allocator->offset = 0;
		memory_zero(allocator->memory, allocator->total_size);
	}
}

