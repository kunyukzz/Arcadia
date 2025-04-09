#include "engine/core/logger.h"
#include "engine/core/assertion.h"
#include "engine//memory/memory.h"

#include <stdlib.h>
#include <stdio.h>

int main(void) {
	/* Test custom logging */
	ar_FATAL("Test Message: %f", 3.14f);
	ar_ERROR("Test Message: %f", 3.14f);
	ar_WARNING("Test Message: %f", 3.14f);
	ar_INFO("Test Message: %f", 3.14f);
	ar_DEBUG("Test Message: %f", 3.14f);
	printf("######################################\n");

	/* Get required size */
    uint64_t memory_requirement = 0;
    memory_init(&memory_requirement, NULL);
    void *memory_block = memory_alloc(memory_requirement, MEMTAG_APPLICATION);
    memory_init(&memory_requirement, memory_block);

    /* Allocate test memory with a couple tags */
    void *a = memory_alloc(2048, MEMTAG_APPLICATION);
    void *b = memory_alloc(1024 * 1024, MEMTAG_RENDERER); // 1MB
    void *c = memory_alloc(512, MEMTAG_STRING);

    /* Print memory stats */
    const char *stats = memory_debug_stats(memory_block);
	ar_INFO(stats);

    /* Free memory */
    memory_free(a, 2048, MEMTAG_APPLICATION);
    memory_free(b, 1024 * 1024, MEMTAG_RENDERER);
    memory_free(c, 512, MEMTAG_STRING);

    memory_shut(memory_block);
	return 0;
}

