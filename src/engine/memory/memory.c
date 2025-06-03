#include "engine/memory/memory.h"

#include "engine/core/logger.h"
#include "engine/core/ar_strings.h"
#include "engine/memory/dyn_alloc.h"
#include "engine/platform/platform.h"

static const char *memtag_string[MEMTAG_MAX_TAGS] = {
    "MEMTAG_UNKNOWN",
	"MEMTAG_ARENA_ALLOCATOR",
	"MEMTAG_STACK_ALLOCATOR",
    "MEMTAG_ARRAY",
    "MEMTAG_DYN_ARRAY",
    "MEMTAG_STRING",
    "MEMTAG_APPLICATION",
    "MEMTAG_RENDERER",
    "MEMTAG_GAME",
	"MEMTAG_TEXTURE",
	"MEMTAG_MATERIAL",
	"MEMTAG_RESOURCE",
    "MEMTAG_TRANSFORM",
    "MEMTAG_ENTITY",
    "MEMTAG_ENTITY_NODE",
    "MEMTAG_SCENE"
};

struct mem_status {
	uint64_t total_allocated;
	uint64_t tagged_alloc_count[MEMTAG_MAX_TAGS];
	uint64_t tagged_allocation[MEMTAG_MAX_TAGS];
};

typedef struct memory_state_t {
	struct mem_status status;
	memory_sys_config_t config;
	uint64_t alloc_count;
	uint64_t alloc_mem_require;
	dyn_alloc_t allocator;
	void *allocator_block;
} memory_state_t;

static memory_state_t *p_state;

b8 memory_init(memory_sys_config_t config) {
	uint64_t state_memory_require = sizeof(memory_state_t);
	uint64_t alloc_req = 0;
	dyn_alloc_init(config.total_alloc_size, &alloc_req, 0, 0);

	void *block = platform_allocate(state_memory_require + alloc_req, false);
	if (!block) {
		ar_FATAL("Memory allocation failed.");
		return false;
	}

	p_state = (memory_state_t *)block;
	p_state->config = config;
	p_state->alloc_count = 0;
	p_state->alloc_mem_require = alloc_req;
	memory_zero(&p_state->status, sizeof(p_state->status));

	p_state->allocator_block = ((void *)((char *)block + state_memory_require));

    if (!dyn_alloc_init(config.total_alloc_size,
						&p_state->alloc_mem_require,
                        p_state->allocator_block, &p_state->allocator)) {
        ar_FATAL("Memory unable to setup internal allocator.");
        return false;
    }

    ar_INFO("Memory System Initialized. Allocated %llu bytes",
                                config.total_alloc_size);
    return true;
}

void memory_shut() {
    if (p_state) {
        dyn_alloc_shut(&p_state->allocator);
        platform_free(p_state,
                      p_state->alloc_mem_require + sizeof(memory_state_t));
    }
    p_state = 0;
}

void *memory_alloc_debug(uint64_t size, mem_tag_t tag, const char *file,
                         int line, const char *func) {
    if (tag == MEMTAG_UNKNOWN)
        ar_WARNING("Memory allocation with MEMTAG_UNKNOWN at %s:%d (%s)", file,
                   line, func);

    void *block = 0;
    if (p_state) {
        p_state->status.total_allocated += size;
        p_state->status.tagged_allocation[tag] += size;
        p_state->status.tagged_alloc_count[tag]++;
        p_state->alloc_count++;
        block = dyn_alloc_allocate(&p_state->allocator, size);
    } else {
        block = platform_allocate(size, false);
    }

    if (block) {
        memory_set(block, 0, size);
        return block;
    }
    return 0;
}

void memory_free(void *block, uint64_t size, mem_tag_t tag) {
	if (!block) {
		return;
	}

    if (p_state) {
        p_state->status.total_allocated -= size;
        p_state->status.tagged_allocation[tag] -= size;
        p_state->status.tagged_alloc_count[tag]--;

        b8 result = dyn_alloc_free(&p_state->allocator, block, size);
        if (!result)
            platform_free(block, false);
    } else {
        platform_free(block, false);
    }
}

void *memory_zero(void *block, uint64_t size) {
	return memset(block, 0, size);
}

void *memory_copy(void *target, const void *source, uint64_t size) {
	return memcpy(target, source, size);
}

void *memory_set(void *target, int32_t value, uint64_t size) {
	return memset(target, value, size);
}

char *memory_debug_stats(void) {
	const uint64_t Gib = 1024 * 1024 * 1024;
	const uint64_t Mib = 1024 * 1024;
	const uint64_t Kib = 1024;

	char buffer[8000] = "System Memory Used:\n";
//	uint64_t offset = strlen(buffer);
	uint64_t offset = string_length(buffer);

	for (uint32_t i = 0; i < MEMTAG_MAX_TAGS; ++i) {
		char unit[4] = "Xib";
		uint32_t count = 0;
		float amount = 1.0f;

		if (p_state->status.tagged_allocation[i] >= Gib) {
			unit[0] = 'G';
			amount = p_state->status.tagged_allocation[i] / (float)Gib;
		} else if (p_state->status.tagged_allocation[i] >= Mib) {
			unit[0] = 'M';
			amount = p_state->status.tagged_allocation[i] / (float)Mib;
		} else if (p_state->status.tagged_allocation[i] >= Kib) {
			unit[0] = 'K';
			amount = p_state->status.tagged_allocation[i] / (float)Kib;
		} else {
			unit[0] = 'B';
			amount = p_state->status.tagged_allocation[i];
		}
		count = p_state->status.tagged_alloc_count[i];

#if OS_LINUX

		int32_t length = snprintf(buffer + offset, 8000,
			"--> %s: [\x1b[33m%u\x1b[0m] \x1b[34m%.2f%s\x1b[0m\n",
			memtag_string[i], count, amount, unit);

#elif OS_WINDOWS
		
		int32_t length = snprintf(buffer + offset, 8000, "--> %s: [%u] %.2f%s\n",
				memtag_string[i], count, amount, unit);

#endif

		offset += (uint32_t)length;

		if (offset >= sizeof(buffer))
			break;
	}
	
	char *out = string_duplicate(buffer);
	return out;
}

uint64_t get_mem_alloc_count(void) {
    if (p_state)
        return p_state->alloc_count;

    return 0;
}
