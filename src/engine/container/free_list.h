#ifndef __FREE_LIST_H__
#define __FREE_LIST_H__

#include "engine/define.h"

typedef struct freelist_t {
	void *memory;
} freelist_t;

_arapi void freelist_init(uint64_t total_size, uint64_t *mem_require,
                          void *memory, freelist_t *freelist);

_arapi void freelist_shut(freelist_t *freelist);
_arapi b8 freelist_block_alloc(freelist_t *freelist, uint64_t size,
                               uint64_t *offset);

_arapi b8 freelist_block_free(freelist_t *freelist, uint64_t size,
                              uint64_t offset);

_arapi void     freelist_clear(freelist_t *freelist);
_arapi uint64_t freelist_space_free(freelist_t *freelist);
#endif //__FREE_LIST_H__
