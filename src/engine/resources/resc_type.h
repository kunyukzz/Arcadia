#ifndef __RESOURCE_TYPE_H__
#define __RESOURCE_TYPE_H__

#include "engine/math/math_type.h"

#define TEXTURE_NAME_MAX_LENGTH 512
#define MATERIAL_NAME_MAX_LENGTH 256
#define GEOMETRY_NAME_MAX_LENGTH 256

typedef enum resource_type_t {
	RESC_TYPE_TEXT,
	RESC_TYPE_BINARY,
	RESC_TYPE_IMAGE,
	RESC_TYPE_MATERIAL,
	RESC_TYPE_STATIC_MESH,
	RESC_TYPE_SHADER,
	RESC_TYPE_CUSTOM
} resource_type_t;

typedef struct resource_t {
	uint32_t id_loader;
	const char *name;
	char *full_path;
	uint64_t data_size;
	void *data;
} resource_t;

typedef struct image_resc_data_t {
	uint8_t channel_count;
	uint32_t width;
	uint32_t height;
	uint8_t *pixels;
} image_resc_data_t;

/* ================================ Texture ================================= */
typedef struct texture_t {
	uint32_t id;
	uint32_t width;
	uint32_t height;
	uint32_t gen;

	uint8_t channel_count;
	char name[TEXTURE_NAME_MAX_LENGTH];
	b8 has_transparent;

	void *internal_data;
} texture_t;

typedef enum texture_use_t {
	TEXTURE_USE_UNKNOWN = 0x00,
	TEXTURE_USE_MAP_DIFFUSE
} texture_use_t;

typedef struct texture_map_t {
	texture_t *texture;
	texture_use_t used;
} texture_map_t;

/* =============================== Material ================================= */
/*
typedef enum material_type_t {
	MATERIAL_TYPE_WORLD,
	MATERIAL_TYPE_UI
} material_type_t;
*/

typedef struct material_config_t {
	char name[MATERIAL_NAME_MAX_LENGTH];
	char *shader_name;
	b8 auto_release;
	vec4 diffuse_color;
	char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
} material_config_t;

typedef struct material_t {
	uint32_t id;
	uint32_t gen;
	uint32_t internal_id;
	uint32_t shader_id;
	char name[MATERIAL_NAME_MAX_LENGTH];
	vec4 diffuse_color;
	texture_map_t diffuse_map;
} material_t;

/* =============================== Geometry ================================= */
typedef struct geometry_t {
	uint32_t id;
	uint32_t gen;
	uint32_t internal_id;
	char name[GEOMETRY_NAME_MAX_LENGTH];
	material_t *material;
} geometry_t;

/* ================================ Shader ================================== */
typedef enum shader_stage_t {
	SHADER_STAGE_VERTEX = 0x00000001,
	SHADER_STAGE_GEOMETRY = 0x00000002,
	SHADER_STAGE_FRAGMENT = 0x00000004,
	SHADER_STAGE_COMPUTE = 0x00000008
} shader_stage_t;

typedef enum shader_attr_type_t {
	SHADER_ATTR_FLOAT32 = 0U,
	SHADER_ATTR_FLOAT32_2 = 1U,
	SHADER_ATTR_FLOAT32_3 = 2U,
	SHADER_ATTR_FLOAT32_4 = 3U,
	SHADER_ATTR_MATRIX_4 = 4U,
	SHADER_ATTR_INT8 = 5U,
	SHADER_ATTR_UINT8 = 6U,
	SHADER_ATTR_INT16 = 7U,
	SHADER_ATTR_UINT16 = 8U,
	SHADER_ATTR_INT32 = 9U,
	SHADER_ATTR_UINT32 = 10U
} shader_attr_type_t;

typedef enum shader_uniform_type_t {
    SHADER_UNIFORM_FLOAT32 = 0U,
    SHADER_UNIFORM_FLOAT32_2 = 1U,
    SHADER_UNIFORM_FLOAT32_3 = 2U,
    SHADER_UNIFORM_FLOAT32_4 = 3U,
    SHADER_UNIFORM_INT8 = 4U,
    SHADER_UNIFORM_UINT8 = 5U,
    SHADER_UNIFORM_INT16 = 6U,
    SHADER_UNIFORM_UINT16 = 7U,
    SHADER_UNIFORM_INT32 = 8U,
    SHADER_UNIFORM_UINT32 = 9U,
    SHADER_UNIFORM_MATRIX_4 = 10U,
    SHADER_UNIFORM_SAMPLER = 11U,
    SHADER_UNIFORM_CUSTOM = 255U
} shader_uniform_type_t;

typedef enum shader_scope_t {
	SHADER_SCOPE_GLOBAL = 0,
	SHADER_SCOPE_INSTANCE = 1,
	SHADER_SCOPE_LOCAL = 2
} shader_scope_t;

typedef struct shader_attr_config_t {
	shader_attr_type_t type;
	shader_scope_t scope;
	uint32_t location;
	uint8_t name_length;
	uint8_t size;
	char *name;
} shader_attr_config_t;

typedef struct shader_uniform_config_t {
	shader_uniform_type_t type;
	shader_scope_t scope;
	uint32_t location;
	uint8_t size;
	uint8_t name_length;
	char *name;
} shader_uniform_config_t;

typedef struct shader_config_t {
	shader_attr_config_t *attributes;
	shader_uniform_config_t *uniforms;
	shader_stage_t *stages;
	uint8_t attr_count;
	uint8_t uniform_count;
	uint8_t stage_count;
	b8 use_instances;
	b8 use_locals;
	char *name;
	char *renderpass_name;
	char **stage_names;
	char **stage_filenames;
} shader_config_t;

#endif // __RESOURCE_TYPE_H__
