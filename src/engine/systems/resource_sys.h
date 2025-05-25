#ifndef __RESOURCE_SYSTEM_H__
#define __RESOURCE_SYSTEM_H__

#include "engine/resources/resc_type.h"

typedef struct resource_sys_config_t {
	uint32_t max_loader_count;
	char *base_path;
} resource_sys_config_t;

typedef struct resource_loader_t {
	uint32_t id;
	resource_type_t type;
	const char *custom_type;
	const char *type_path;
	const char *file_exts;
	b8 (*load)(struct resource_loader_t *self, const char *name, resource_t *resource);
	void (*unload)(struct resource_loader_t *self, resource_t *resource);
} resource_loader_t;

b8 resource_sys_init(uint64_t *memory_require, void *state,
                       resource_sys_config_t config);
void resource_sys_shut(void *state);

b8 resource_sys_reg_loader(resource_loader_t loader);
b8 resource_sys_load(const char *name, resource_type_t type, resource_t *resc);
b8 resource_sys_load_custom(const char *name, const char *custom_type,
                            resource_t *resc);

void resource_sys_unload(resource_t *resc);
const char *resource_sys_base_path(void);

#endif //__RESOURCE_SYSTEM_H__
