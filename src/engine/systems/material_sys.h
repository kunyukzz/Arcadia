#ifndef __MATERIAL_SYSTEM_H__
#define __MATERIAL_SYSTEM_H__

#include "engine/define.h"
#include "engine/resources/resc_type.h"

#define DEFAULT_MATERIAL_NAME "Default"

typedef struct material_sys_config_t {
	uint32_t max_material_count;
} material_sys_config_t;

typedef struct material_config_t {
	vec4 diffuse_color;
	b8 auto_release;
	char name[MATERIAL_NAME_MAX_LENGTH];
	char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
} material_config_t;

b8   material_sys_init(uint64_t *memory_require, void *state,
                       material_sys_config_t config);
void material_sys_shut(void *state);

material_t *material_sys_get_default(void);
material_t *material_sys_acquire(const char *name);
material_t *material_sys_acquire_from_config(material_config_t config);
void        material_sys_release(const char *name);

#endif //__MATERIAL_SYSTEM_H__
