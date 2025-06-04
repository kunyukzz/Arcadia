#ifndef __SHADER_SYSTEM_H__
#define __SHADER_SYSTEM_H__

#include "engine/define.h"
#include "engine/renderer/renderer_type.h"
#include "engine/container/hashtable.h"

typedef struct shader_sys_config_t {
	uint16_t max_shader_count;
	uint8_t max_uniform_count;
	uint8_t max_global_texture;
	uint8_t max_instance_texture;
} shader_sys_config_t;

typedef enum shader_state_t {
	SHADER_NOT_CREATED,
	SHADER_UNINITIALIZED,
	SHADER_INITIALIZED
} shader_state_t;

typedef struct shader_uniform_t {
	shader_uniform_type_t type;
	shader_scope_t scope;
	uint64_t offset;
	uint16_t location;
	uint16_t index;
	uint16_t size;
	uint8_t set_index;
} shader_uniform_t;

typedef struct shader_attribute_t {
	shader_attr_type_t type;
	uint32_t size;
	char *name;
} shader_attribute_t;

typedef struct shader_t {
	shader_uniform_t *uniforms;
	shader_attribute_t *attributes;
	shader_state_t state;
	shader_scope_t bound_scope;

	texture_t **global_textures;
	range push_const_ranges[32];

	hashtable_t uniform_lookup;

	uint64_t req_ubo_alignment;
	uint64_t global_ubo_size;
	uint64_t global_ubo_stride;
	uint64_t global_ubo_offset;

	uint64_t ubo_size;
	uint64_t ubo_stride;
	uint64_t push_const_size;
	uint64_t push_const_stride;

	uint32_t id;
	uint32_t bound_inst_id;
	uint32_t bound_ubo_offset;

	uint16_t attr_stride;

	uint8_t inst_texture_count;
	uint8_t push_const_range_count;

	b8 use_instances;
	b8 use_locals;
	char *name;

	void *hashtable_block;
	void *internal_data;
} shader_t;

b8 shader_sys_init(uint64_t *memory_require, void *memory,
                   shader_sys_config_t config);

void shader_sys_shut(void *state);

_arapi b8 shader_sys_create(const shader_config_t *config);
_arapi uint32_t shader_sys_get_id(const char *shader_name);
_arapi shader_t *shader_sys_get_by_id(uint32_t shader_id);
_arapi shader_t *shader_sys_get(const char *shader_name);

_arapi b8 shader_sys_use(const char *shader_name);
_arapi b8 shader_sys_use_id(uint32_t shader_id);

_arapi uint16_t shader_sys_uniform_idx(shader_t *shader, const char *uniform_name);
_arapi b8 shader_sys_uniform_set(const char *uniform_name, const void *value);
_arapi b8 shader_sys_uniform_set_idx(uint16_t index, const void *value);

_arapi b8 shader_sys_sampler_set(const char *sampler_name, const texture_t *tex);
_arapi b8 shader_sys_sampler_set_idx(uint16_t index, const struct texture_t *tex);

_arapi b8 shader_sys_apply_global();
_arapi b8 shader_sys_apply_instance();
_arapi b8 shader_sys_bind_instance(uint32_t instance_id);

#endif //__SHADER_SYSTEM_H__
