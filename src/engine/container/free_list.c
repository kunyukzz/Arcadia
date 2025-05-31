#include "engine/container/free_list.h"

#include "engine/core/logger.h"
#include "engine/memory/memory.h"

typedef struct freelist_node_t {
	uint64_t offset;
	uint64_t size;
	struct freelist_node_t *next;
} freelist_node_t;

typedef struct internal_state_t {
	uint64_t total_size;
	uint64_t max_entry;
	freelist_node_t *head;
	freelist_node_t *nodes;
} internal_state_t;

/* ========================= PRIVATE FUNCTION =============================== */
/* ========================================================================== */
freelist_node_t *get_node(freelist_t *freelist) {
	internal_state_t *state = freelist->memory;

	for (uint64_t i = 0; i < state->max_entry; ++i) {
		if (state->nodes[i].offset == INVALID_ID) {
			return &state->nodes[i];
		}
	}

	return 0;
}

void return_node(freelist_t *freelist, freelist_node_t *node) {
	(void)freelist;
	node->offset = INVALID_ID;
	node->size = INVALID_ID;
	node->next = 0;
}

/* ========================================================================== */
/* ========================================================================== */
void freelist_init(uint64_t total_size, uint64_t *mem_require,
                          void *memory, freelist_t *freelist) {
	uint64_t max_entry = (total_size / sizeof(void *));
	*mem_require = sizeof(internal_state_t) + (sizeof(freelist_node_t) * max_entry);
	if (!memory) 
		return;

	uint64_t mem_min = (sizeof(internal_state_t) + sizeof(freelist_node_t)) * 8;
    if (total_size < mem_min) {
        ar_WARNING("freelist not efficient with amount of memory less than "
                   "%iB. Not recommended to use this structure");
    }

	freelist->memory = memory;

	memory_zero(freelist->memory, *mem_require);
	internal_state_t *state = freelist->memory;
	state->nodes = (void *)((char *)freelist->memory + sizeof(internal_state_t));
	state->max_entry = max_entry;
	state->total_size = total_size;
	state->head = &state->nodes[0];
	state->head->offset = 0;
	state->head->size = total_size;
	state->head->next = 0;

	for (uint64_t i = 1; i < state->max_entry; ++i) {
		state->nodes[i].offset = INVALID_ID;
		state->nodes[i].size = INVALID_ID;
	}
}

void freelist_shut(freelist_t *freelist) {
    if (freelist && freelist->memory) {
        internal_state_t *state = freelist->memory;
        memory_zero(freelist->memory,
                    sizeof(internal_state_t) +
                        sizeof(freelist_node_t) * state->max_entry);
        freelist->memory = 0;
    }
}

b8 freelist_block_alloc(freelist_t *freelist, uint64_t size,
                               uint64_t *offset) {
	if (!freelist || !offset || !freelist->memory) {
		return false;
	}

	internal_state_t *state = freelist->memory;
	freelist_node_t *node = state->head;
	freelist_node_t *prev = 0;

	while (node) {
		if (node->size == size) {
			*offset = node->offset;
			freelist_node_t *node_ret = 0;
			if (prev) {
				prev->next = node->next;
				node_ret = node;
			} else {
				node_ret = state->head;
				state->head = node->next;
			}

			return_node(freelist, node_ret);
			return true;
		} else if (node->size > size) {
			*offset = node->offset;
			node->size -= size;
			node->offset += size;
			return true;
		}

		prev = node;
		node = node->next;
	}

	uint64_t free_space = freelist_space_free(freelist);
    ar_WARNING("freelist_block - no block with enough free space found. "
               "Requested: %lluB, available: %lluB",
               size, free_space);

    return false;
}

b8 freelist_block_free(freelist_t *freelist, uint64_t size, uint64_t offset) {
    if (!freelist || !freelist->memory || !size)
        return false;

    internal_state_t *state = freelist->memory;
    freelist_node_t  *node  = state->head;
    freelist_node_t  *prev  = 0;

    if (!node) {
        freelist_node_t *new_node = get_node(freelist);
        new_node->offset          = offset;
        new_node->size            = size;
        new_node->next            = 0;
        state->head               = new_node;
        return true;
    } else {
        while (node) {
            if (node->offset == offset) {
                node->size += size;

                if (node->next &&
                    node->next->offset == node->offset + node->size) {
                    node->size += node->next->size;
                    freelist_node_t *next = node->next;
                    node->next            = node->next->next;
                    return_node(freelist, next);
                }
                return true;
            } else if (node->offset > offset) {
                freelist_node_t *new_node = get_node(freelist);
                new_node->offset          = offset;
                new_node->size            = size;

                /* If there is previous node, new node should be inserted
                 * between this and it */
                if (prev) {
                    prev->next     = new_node;
                    new_node->next = node;
                } else {
                    new_node->next = node;
                    state->head    = new_node;
                }

                /* Double check next node to see if can be joined */
                if (new_node->next && new_node->offset + new_node->size ==
                                          new_node->next->offset) {
                    new_node->size += new_node->next->size;
                    freelist_node_t *garbage = new_node->next;
                    new_node->next           = garbage->next;
                    return_node(freelist, garbage);
                }

                /* Double check previous node to see if new node can be joined
                 */
                if (prev && prev->offset + prev->size == new_node->offset) {
                    prev->size += new_node->size;
                    freelist_node_t *garbage = new_node;
                    prev->next               = garbage->next;
                    return_node(freelist, garbage);
                }

                return true;
            }

            prev = node;
            node = node->next;
        }
    }

    ar_WARNING("Unable to find block to be freed.");
    return false;
}

void freelist_clear(freelist_t *freelist) {
	if (!freelist || !freelist->memory)
		return;

	internal_state_t *state = freelist->memory;
	for (uint64_t i = 0; i < state->max_entry; ++i) {
		state->nodes[i].offset = INVALID_ID;
		state->nodes[i].size = INVALID_ID;
	}

	state->head->offset = 0;
	state->head->size = state->total_size;
	state->head->next = 0;
}

uint64_t freelist_space_free(freelist_t *freelist) {
	if (!freelist || !freelist->memory)
		return 0;

	uint64_t run_total = 0;
	internal_state_t *state = freelist->memory;
	freelist_node_t *node = state->head;

	while (node) {
		run_total += node->size;
		node = node->next;
	}

	return run_total;
}

