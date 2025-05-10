#include "engine/container/hashtable.h"

#include "engine/core/logger.h"
#include "engine/memory/memory.h"

uint64_t hash_name(const char *name, uint32_t element_count) {
	// use prime number to avoiding collisions.
	static const uint64_t multi = 97;

	unsigned const char *us;
	uint64_t hash = 0;

	for (us = (unsigned const char *)name; *us; us++) {
		hash = hash * multi + *us;
	}

	// modulate against the size of table
	hash %= element_count;

	return hash;
}

void hashtable_init(uint64_t element_size, uint32_t element_count, void *memory,
                    b8 is_pointer_type, hashtable_t *table) {
    if (!memory || !table) {
        ar_ERROR("Failed to create hashtable. Pointer to memory are required");
        return;
    }

    if (!element_count || !element_size) {
        ar_ERROR("Count & Size must be positive non-zero value.");
        return;
    }

    // TODO: might be require an allocator & allocate this memory instead.
    table->memory          = memory;
    table->element_count   = element_count;
    table->element_size    = element_size;
    table->is_pointer_type = is_pointer_type;
    memory_zero(table->memory, element_count * element_size);
}

void hashtable_shut(hashtable_t *table) {
	if (table) {
		memory_zero(table, sizeof(hashtable_t));
	}
}

b8 hashtable_set(hashtable_t *table, const char *name, void *value) {
    if (!table || !name || !value) {
        ar_ERROR("hashtable_set require table, name, value to exist");
        return false;
    }

    if (table->is_pointer_type) {
        ar_ERROR("Hashtable should not be used with tables that have pointer "
                 "types. Use hashtable_set_ptr instead.");
        return false;
    }

    uint64_t hash = hash_name(name, table->element_count);
    memory_copy((char *)table->memory + (table->element_size * hash), value,
                table->element_size);
    return true;
}

b8 hashtable_set_ptr(hashtable_t *table, const char *name, void **value) {
    if (!table || !name) {
        ar_ERROR("hashtable_set_ptr require table and name to exist.");
        return false;
    }

    if (!table->is_pointer_type) {
        ar_ERROR(
            "hashtable_set_ptr should not be used with table that do not have "
            "pointer types. Use hashtable_set instead.");
        return false;
    }

    uint64_t hash                  = hash_name(name, table->element_count);
    ((void **)table->memory)[hash] = value ? *value : 0;
    return true;
}

b8 hashtable_get(hashtable_t *table, const char *name, void *value) {
    if (!table || !name || !value) {
        ar_ERROR("hashtable_get require table, name and value to exist");
        return false;
    }

    if (table->is_pointer_type) {
        ar_ERROR("hashtable_get should not be used with tables that have "
                 "pointer types. Use hashtable_get_ptr instead");
        return false;
    }

    uint64_t hash = hash_name(name, table->element_count);
    memory_copy(value, (char *)table->memory + (table->element_size * hash),
                table->element_size);
    return true;
}

b8 hashtable_get_ptr(hashtable_t *table, const char *name, void **value) {
    if (!table || !name) {
        ar_ERROR("hashtable_get_ptr require table and name to exist");
        return false;
    }

    if (!table->is_pointer_type) {
        ar_ERROR("hashtable_get_ptr should not be used with table that do not "
                 "have pointer types. Use hashtable_get instead");
        return false;
    }

    uint64_t hash = hash_name(name, table->element_count);
    *value        = ((void **)table->memory)[hash];
    return true;
}

b8 hashtable_fill(hashtable_t *table, void *value) {
    if (!table || !value) {
        ar_WARNING("hashtable_fill require table and value to exist");
        return false;
    }

    if (table->is_pointer_type) {
        ar_ERROR("hashtable_fill should not be used with tables that have "
                 "pointer types");
        return false;
    }

    for (uint32_t i = 0; i < table->element_count; ++i) {
        memory_copy((char *)table->memory + (table->element_size * i), value,
                    table->element_size);
    }

    return true;
}
