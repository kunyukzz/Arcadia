#ifndef __DYNAMIC_ARRAY_H__
#define __DYNAMIC_ARRAY_H__

#include "engine/define.h"

enum {
	DYN_ARRAY_CAPACITY, 	// Total allocated elements
	DYN_ARRAY_LENGTH, 		// Number elements currently used
	DYN_ARRAY_STRIDE, 		// Size of elements in byte
	DYN_ARRAY_FIELD_LENGTH 	// Total metadata fields
};

#define DYN_ARRAY_META(arr) ((uint64_t *)(arr) - DYN_ARRAY_FIELD_LENGTH)

void *_array_create(uint64_t length, uint64_t stride);
void _array_destroy(void *array);

uint64_t _array_get_field(void *array, uint64_t field);
void _array_set_field(void *array, uint64_t field, uint64_t value);
void *_array_resize(void *array);

void *_array_push(void *array, const void *value_ptr);
void _array_pop(void *array, void *target);

void *_array_pop_at(void *array, uint64_t index, void *target);
void *_array_insert_at(void *array, uint64_t index, void *value_ptr);

#define DYN_ARRAY_DEF_CAPACITY 0x01
#define DYN_ARRAY_RESIZE_FACTOR 0x02

#define dyn_array_create(type) _array_create(DYN_ARRAY_DEF_CAPACITY, sizeof(type))
#define dyn_array_reserved(type, capacity) _array_create(capacity, sizeof(type))
#define dyn_array_destroy(array) _array_destroy(array)

#define dyn_array_push(array, value) { 							\
		__typeof__(value) temp = (value); 						\
		(array) = _array_push((array), &temp);}


#define dyn_array_insert_at(array, index, value) { 				\
		__typeof__(value) temp = value; 						\
		array = _array_insert_at(array, index, &temp);}

#define dyn_array_pop(array, value_ptr) _array_pop(array, value_ptr)
#define dyn_array_pop_at(array, index, value_ptr) _array_pop_at(array, index, value_ptr)

#define dyn_array_clear(array) _array_set_field(array, DYN_ARRAY_LENGTH, 0)
#define dyn_array_capacity(array) _array_get_field(array, DYN_ARRAY_CAPACITY)
#define dyn_array_length(array) _array_get_field(array, DYN_ARRAY_LENGTH)
#define dyn_array_stride(array) _array_get_field(array, DYN_ARRAY_STRIDE)
#define dyn_array_length_set(array, value) _array_set_field(array, DYN_ARRAY_LENGTH, value)

#endif //__DYNAMIC_ARRAY_H__
