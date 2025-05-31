#include "engine/memory/dyn_alloc.h"

#include "engine/container/free_list.h"
#include "engine/core/logger.h"
#include "engine/memory/memory.h"

typedef struct dyn_alloc_state_t {
	uint64_t total_size;
	freelist_t freelist;
	void *freelist_block;
	void *mem_block;
} dyn_alloc_state_t;

b8 dyn_alloc_init(uint64_t total_size, uint64_t *mem_require,
                         void *memory, dyn_alloc_t *dyn_alloc) {
	if (total_size < 1) {
		ar_ERROR("dyn_alloc_init - cannot have total size of 0.");
		return false;
	}

	if (!mem_require) {
		ar_ERROR("dyn_alloc_init - require 'memory_requirement' to exist");
		return false;
	}

	uint64_t freelist_req = 0;
	freelist_init(total_size, &freelist_req, 0, 0);
	*mem_require = freelist_req + sizeof(dyn_alloc_state_t) + total_size;

	if (!memory)
		return true;

	dyn_alloc->memory = memory;
	dyn_alloc_state_t *state = dyn_alloc->memory;
	state->total_size = total_size;
    state->freelist_block =
        (void *)((char *)dyn_alloc->memory + sizeof(dyn_alloc_state_t));
    state->mem_block = (void *)((char *)state->freelist_block + freelist_req);

    /* Actual Freelist create */
    freelist_init(total_size, &freelist_req, state->freelist_block,
                  &state->freelist);

    memory_zero(state->mem_block, total_size);
	return true;
}

b8 dyn_alloc_shut(dyn_alloc_t *dyn_alloc) {
	if (dyn_alloc) {
		dyn_alloc_state_t *state = dyn_alloc->memory;
		freelist_shut(&state->freelist);
		memory_zero(state->mem_block, state->total_size);
		state->total_size = 0;
		dyn_alloc->memory = 0;
		return true;
	}

	ar_WARNING("dyn_alloc_shut - require pointer to an allocator. Failed");
	return false;
}

void *dyn_alloc_allocate(dyn_alloc_t *dyn_alloc, uint64_t size) {
	if (dyn_alloc && size) {
		dyn_alloc_state_t *state = dyn_alloc->memory;
		uint64_t offset = 0;

		if (freelist_block_alloc(&state->freelist, size, &offset)) {
			void *block = (void *)((char *)state->mem_block + offset);
			return block;
		} else {
            ar_ERROR("dyn_alloc_allocate - no blocks of memory large enough to "
                     "allocate");
            uint64_t avail = freelist_space_free(&state->freelist);
            ar_ERROR("Requested Size: %llu, available: %llu", size, avail);

			// TODO: Report Fragmentation
			return 0;
		}
	}

	ar_ERROR("dyn_alloc_allocate - require a valid allocator & size");
	return 0;
}

void dyn_alloc_free(dyn_alloc_t *dyn_alloc, void *block, uint64_t size) {
    if (!dyn_alloc || !block) {
        ar_ERROR("dyn_alloc_free - require both a valid allocator (0x%p) and "
                 "block (0x%p) to be freed",
                 dyn_alloc, block);
        return;
    }

    dyn_alloc_state_t *state = dyn_alloc->memory;
    if ((char *)block < (char *)state->mem_block ||
        (char *)block > (char *)state->mem_block + state->total_size) {
        void *end_block = (void *)((char *)state->mem_block + state->total_size);
        ar_ERROR("dyn_alloc_free - try to release block (0x%p) outside of "
                 "allocator range (0x%p)-(0x%p)",
                 block, state->mem_block, end_block);
        return;
    }

	uint64_t offset = (uint64_t)((char *)block - (char *)state->mem_block);
	if (!freelist_block_free(&state->freelist, size, offset)) {
		ar_ERROR("dyn_alloc_free - Failed");
		return;
	}
}

uint64_t dyn_alloc_free_space(dyn_alloc_t *dyn_alloc) {
	dyn_alloc_state_t *state = dyn_alloc->memory;
	return freelist_space_free(&state->freelist);
}

