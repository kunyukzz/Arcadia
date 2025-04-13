#include "engine/container/dyn_array.h"
#include "engine/memory/memory.h"
#include "engine/core/logger.h"

#include <stdlib.h>

void *_array_create(uint64_t length, uint64_t stride) {
	uint64_t size_header = DYN_ARRAY_FIELD_LENGTH * sizeof(uint64_t);
	uint64_t size_array 	= length * stride;

	uint64_t *array = memory_alloc(size_header + size_array, MEMTAG_DYN_ARRAY);
	if (!array) 
		return NULL;

	memory_set(array, 0, size_header + size_array);

	array[DYN_ARRAY_CAPACITY] 	= length;
	array[DYN_ARRAY_LENGTH] 	= 0;
	array[DYN_ARRAY_STRIDE] 	= stride;

	return (void *)(array + DYN_ARRAY_FIELD_LENGTH);
}

void _array_destroy(void *array) {
	uint64_t *header = (uint64_t *)array - DYN_ARRAY_FIELD_LENGTH;
	uint64_t size_header = DYN_ARRAY_FIELD_LENGTH * sizeof(uint64_t);
	uint64_t size_total = size_header + header[DYN_ARRAY_CAPACITY] * header[DYN_ARRAY_STRIDE];
	memory_free(header, size_total, MEMTAG_DYN_ARRAY);
}

uint64_t _array_get_field(void *array, uint64_t field) {
	uint64_t *header = (uint64_t *)array - DYN_ARRAY_FIELD_LENGTH;
	return header[field];
}

void _array_set_field(void *array, uint64_t field, uint64_t value) {
	uint64_t *header = (uint64_t *)array - DYN_ARRAY_FIELD_LENGTH;
	header[field] = value;
}

void *_array_resize(void *array) {
	uint64_t length = dyn_array_length(array);
	uint64_t stride = dyn_array_stride(array);

	void *temp = _array_create(DYN_ARRAY_RESIZE_FACTOR * dyn_array_capacity(array), stride);
	memory_copy(temp, array, length * stride);

	_array_set_field(temp, DYN_ARRAY_LENGTH, length);
	_array_destroy(array);
	return temp;
}

void *_array_push(void *array, const void *value_ptr) {
	uint64_t length = dyn_array_length(array);
	uint64_t stride = dyn_array_stride(array);
	if (length >= dyn_array_capacity(array))
		array = _array_resize(array);

	uint64_t address = (uint64_t)array;
	address += (length * stride);
	memory_copy((void *)address, value_ptr, stride);
	_array_set_field(array, DYN_ARRAY_LENGTH, length + 1);
	return array;
}

void _array_pop(void *array, void *target) {
	uint64_t length = dyn_array_length(array);
	uint64_t stride = dyn_array_stride(array);
	uint64_t address = (uint64_t)array;

	address += ((length - 1) * stride);
	memory_copy(target, (void *)address, stride);
	_array_set_field(array, DYN_ARRAY_LENGTH, length - 1);
}

void *_array_pop_at(void *array, uint64_t index, void *target) {
	uint64_t length = dyn_array_length(array);
	uint64_t stride = dyn_array_stride(array);
	if (index >= length) {
		ar_WARNING("Index outside of bounds. Length: &lu, Index: %lu.", length, index);
		return array;
	}

	uint64_t address = (uint64_t)array;
	memory_copy(target, (void *)(address + (index * stride)), stride);
	if (index != length - 1)
		memory_copy((void *)(address + (index * stride)), (void *)(address + ((index + 1) * stride)), stride * (length - index));

	_array_set_field(array, DYN_ARRAY_LENGTH, length - 1);
	return array;
}

void *_array_insert_at(void *array, uint64_t index, void *value_ptr) {
	uint64_t length = dyn_array_length(array);
	uint64_t stride = dyn_array_stride(array);
	if (index >= length) {
		ar_WARNING("Index outside of bounds! Length: %lu, Index: %lu.\n", length, index);
		return array;
	}

	if (length >= dyn_array_capacity(array))
		array = _array_resize(array);

	uint64_t address = (uint64_t)array;
	if (index != length - 1)
		memory_copy((void *)(address + ((index - 1) * stride)), (void *)(address + (index * stride)), stride * (length - index));

	memory_copy((void *)(address + (index * stride)), value_ptr, stride);
	_array_set_field(array, DYN_ARRAY_LENGTH, length + 1);
	return array;
}
