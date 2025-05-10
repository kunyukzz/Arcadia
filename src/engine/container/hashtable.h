#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include "engine/define.h"

typedef struct hashtable_t {
	uint64_t element_size;
	uint32_t element_count;
	b8 is_pointer_type;
	void *memory;
} hashtable_t;

_arapi void hashtable_init(uint64_t element_size, uint32_t element_count,
                           void *memory, b8 is_pointer_type,
                           hashtable_t *table);
_arapi void hashtable_shut(hashtable_t *table);

_arapi b8 hashtable_set(hashtable_t *table, const char *name, void *value);
_arapi b8 hashtable_set_ptr(hashtable_t *table, const char *name, void **value);
_arapi b8 hashtable_get(hashtable_t *table, const char *name, void *value);
_arapi b8 hashtable_get_ptr(hashtable_t *table, const char *name, void **value);
_arapi b8 hashtable_fill(hashtable_t *table, void *value);

#endif //__HASHTABLE_H__
