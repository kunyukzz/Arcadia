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

typedef struct material_config_t {
	char name[MATERIAL_NAME_MAX_LENGTH];
	b8 auto_release;
	vec4 diffuse_color;
	char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
} material_config_t;

typedef struct material_t {
	uint32_t id;
	uint32_t gen;
	uint32_t internal_id;
	char name[MATERIAL_NAME_MAX_LENGTH];
	vec4 diffuse_color;
	texture_map_t diffuse_map;
} material_t;

typedef struct geometry_t {
	uint32_t id;
	uint32_t gen;
	uint32_t internal_id;
	char name[GEOMETRY_NAME_MAX_LENGTH];
	material_t *material;
} geometry_t;

#endif // __RESOURCE_TYPE_H__
