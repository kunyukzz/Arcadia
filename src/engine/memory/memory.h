#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "engine/define.h"

typedef enum mem_tag_t {
    MEMTAG_UNKNOWN = 0x00,
	MEMTAG_ARENA_ALLOCATOR,
	MEMTAG_STACK_ALLOCATOR,
    MEMTAG_ARRAY,
    MEMTAG_DYN_ARRAY,
    MEMTAG_STRING,
    MEMTAG_APPLICATION,
    MEMTAG_RENDERER,
    MEMTAG_GAME,
	MEMTAG_TEXTURE,
    MEMTAG_TRANSFORM,
    MEMTAG_ENTITY,
    MEMTAG_ENTITY_NODE,
    MEMTAG_SCENE,
    MEMTAG_MAX_TAGS,
} mem_tag_t;

#define memory_alloc(size, tag)                                                \
  memory_alloc_debug(size, tag, __FILE__, __LINE__, __func__)

void memory_init(uint64_t *required, void *state);
void memory_shut(void *state);

void *memory_alloc_debug(uint64_t size, mem_tag_t tag, const char *file,
                         int line, const char *func);
void memory_free(void *block, uint64_t size, mem_tag_t tag);

void *memory_zero(void *block, uint64_t size);
void *memory_copy(void *target, const void *source, uint64_t size);
void *memory_set(void *target, int32_t value, uint64_t size);

char *memory_debug_stats(void);
uint64_t get_mem_alloc_count(void);
#endif //__MEMORY_H__
