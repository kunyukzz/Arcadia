#ifndef __STACK_ALLOCATOR_H__
#define __STACK_ALLOCATOR_H__

#include "engine/define.h"

typedef struct stack_allocator_t {
	uint64_t total_size;
	uint64_t offset;
	void *memory;
	b8 own_memory;
} stack_allocator_t;

typedef uint64_t stack_marker;
stack_marker stack_get_marker(stack_allocator_t *allocator);
void stack_free_to_marker(stack_allocator_t *allocator, stack_marker marker);

void stack_init(uint64_t total_size, void *memory, stack_allocator_t *allocator);
void stack_shut(stack_allocator_t *allocator);

void *stack_allocate(stack_allocator_t *allocator, uint64_t size);
void *stack_allocate_align(stack_allocator_t *allocator, uint64_t size,
                           uintptr_t alignment);

void stack_reset(stack_allocator_t *allocator);
void stack_hard_reset(stack_allocator_t *allocator);

#endif //__STACK_ALLOCATOR_H__
