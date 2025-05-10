#ifndef __TEXTURE_SYSTEM_H__
#define __TEXTURE_SYSTEM_H__

#include "engine/renderer/renderer_type.h"

typedef struct texture_sys_config_t {
	uint32_t max_texture_count;
} texture_sys_config_t;

#define DEFAULT_TEXTURE_NAME "Default"

b8         texture_sys_init(uint64_t *memory_require, void *state,
                            texture_sys_config_t config);
void       texture_sys_shut(void *state);

texture_t *texture_sys_acquire(const char *name, b8 auto_release);
void       texture_sys_release(const char *name);

texture_t *texture_sys_get_default_tex(void);

#endif //__TEXTURE_SYSTEM_H__
