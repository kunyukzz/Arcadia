#ifndef __GEOMETRY_SYSTEM_H__
#define __GEOMETRY_SYSTEM_H__

#include "engine/renderer/renderer_type.h"

#define DEFAULT_GEOMETRY_NAME "Default"

typedef struct geo_sys_cfg_t {
	uint32_t max_geo_count;
} geo_sys_cfg_t;

typedef struct geo_config_t {
	vertex_3d *vertices;
	uint32_t vertex_count;
	uint32_t idx_count;
	uint32_t *indices;
	char name[GEOMETRY_NAME_MAX_LENGTH];
	char material_name[MATERIAL_NAME_MAX_LENGTH];
} geo_config_t;

b8 geometry_sys_init(uint64_t *memory_require, void *state, geo_sys_cfg_t sys_cfg);
void geometry_sys_shut(void *state);

geometry_t *geometry_sys_acquire_by_id(uint32_t id);
geometry_t *geometry_sys_acquire_by_config(geo_config_t config, b8 auto_release);
geometry_t *geometry_sys_get_default(void);

void geometry_sys_release(geometry_t *geometry);
geo_config_t geometry_sys_gen_plane_config(float width, float height,
                                           uint32_t x_segcount,
                                           uint32_t y_segcount, float tile_x,
                                           float tile_y, const char *name,
                                           const char *material_name);

#endif //__GEOMETRY_SYSTEM_H__
