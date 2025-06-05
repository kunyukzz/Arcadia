#ifndef __MATERIAL_SYSTEM_H__
#define __MATERIAL_SYSTEM_H__

#include "engine/define.h"
#include "engine/resources/resc_type.h"

#define DEFAULT_MATERIAL_NAME "Default"

typedef struct material_sys_config_t {
	uint32_t max_material_count;
} material_sys_config_t;

b8   material_sys_init(uint64_t *memory_require, void *state,
                       material_sys_config_t config);
void material_sys_shut(void *state);

material_t *material_sys_get_default(void);
material_t *material_sys_acquire(const char *name);
material_t *material_sys_acquire_from_config(material_config_t config);
void        material_sys_release(const char *name);

b8 material_sys_apply_global(uint32_t shader_id, const mat4 *projection, const mat4 *view);
b8 material_sys_apply_instance(material_t *material);
b8 material_sys_apply_local(material_t *material, const mat4 *model);

#endif //__MATERIAL_SYSTEM_H__
