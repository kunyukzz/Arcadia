#ifndef __DYNAMIC_ALLOCATOR_H__
#define __DYNAMIC_ALLOCATOR_H__

#include "engine/define.h"

typedef struct dyn_alloc_t {
	void *memory;
} dyn_alloc_t;

_arapi b8 dyn_alloc_init(uint64_t total_size, uint64_t *mem_require,
                         void *memory, dyn_alloc_t *dyn_alloc);

_arapi b8 dyn_alloc_shut(dyn_alloc_t *dyn_alloc);
_arapi void *dyn_alloc_allocate(dyn_alloc_t *dyn_alloc, uint64_t size);
_arapi void dyn_alloc_free(dyn_alloc_t *dyn_alloc, void *block, uint64_t size);
_arapi uint64_t dyn_alloc_free_space(dyn_alloc_t *dyn_alloc);

#endif //__DYNAMIC_ALLOCATOR_H__
