#include "memory.h"
#include "engine/core/logger.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char *memtag_string[MEMTAG_MAX_TAGS] = {
    "MEMTAG_UNKNOWN",
	"MEMTAG_ARENA",
    "MEMTAG_ARRAY",
    "MEMTAG_LINEAR_ALLOCATOR",
    "MEMTAG_DYN_ARRAY",
    "MEMTAG_STRING",
    "MEMTAG_APPLICATION",
    "MEMTAG_RENDERER",
    "MEMTAG_GAME",
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
	uint64_t alloc_count;
} memory_state_t;

static memory_state_t *pmemory_state;

void memory_init(uint64_t *required, void *state) {
	*required = sizeof(memory_state_t);
	pmemory_state = (memory_state_t *)state;
}

void memory_shut(void *state) {
	pmemory_state = 0;
}

void *memory_alloc(uint64_t size, mem_tag_t tag) {
	if (tag == MEMTAG_UNKNOWN)
		ar_WARNING("Memory Allocation called using unknown, reclass this allocation!");
	
	if (pmemory_state) {
		pmemory_state->status.total_allocated += size;
		pmemory_state->status.tagged_allocation[tag] += size;
		pmemory_state->status.tagged_alloc_count[tag]++;
		pmemory_state->alloc_count++;
	}
	
	void *block = malloc(size);
	memset(block, 0, size);

	return block;
}

void memory_free(void *block, uint64_t size, mem_tag_t tag) {
	if (pmemory_state) {
		pmemory_state->status.total_allocated -= size;
		pmemory_state->status.tagged_allocation[tag] -= size;
	}

	free(block);
}

void *memory_zero(void *block, uint64_t size) {

}
void *memory_copy(void *target, const void *source, uint64_t size) {}
void *memory_set(void *target, int32_t value, uint64_t size) {}

void *memory_debug_stats(void *state) {
	const uint64_t Gib = 1024 * 1024 * 1024;
	const uint64_t Mib = 1024 * 1024;
	const uint64_t Kib = 1024;

	static char buffer[8000] = "System Memory Used:\n";
	uint64_t offset = strlen(buffer);

	for (uint32_t i = 0; i < MEMTAG_MAX_TAGS; ++i) {
		char unit[4] = "Xib";
		uint32_t count = 0;
		float amount = 1.0f;

		if (pmemory_state->status.tagged_allocation[i] >= Gib) {
			unit[0] = 'G';
			amount = pmemory_state->status.tagged_allocation[i] / (float)Gib;
		} else if (pmemory_state->status.tagged_allocation[i] >= Mib) {
			unit[0] = 'M';
			amount = pmemory_state->status.tagged_allocation[i] / (float)Mib;
		} else if (pmemory_state->status.tagged_allocation[i] >= Kib) {
			unit[0] = 'K';
			amount = pmemory_state->status.tagged_allocation[i] / (float)Kib;
		} else {
			unit[0] = 'B';
			amount = pmemory_state->status.tagged_allocation[i];
		}
		count = pmemory_state->status.tagged_alloc_count[i];
#if OS_LINUX
		int32_t length = snprintf(buffer + offset, 8000, "--> %s: [\x1b[33m%u\x1b[0m] \x1b[34m%.2f%s\x1b[0m\n",
				memtag_string[i], count, amount, unit);
#elif OS_WINDOWS
		int32_t length = snprintf(buffer + offset, 8000, "--> %s: [%u] %.2f%s\n",
				memtag_string[i], count, amount, unit);
#endif
		offset += (uint32_t)length;

		if (offset >= sizeof(buffer))
			break;
	}

	return buffer;
}
