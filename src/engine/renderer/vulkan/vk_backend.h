#ifndef __VULKAN_BACKEND_H__
#define __VULKAN_BACKEND_H__

#include "engine/renderer/renderer_be.h"

b8 vk_backend_init(render_backend_t *backend, const char *name);
void vk_backend_shut(render_backend_t *backend);
void vk_backend_resize(render_backend_t *backend, uint32_t width, uint32_t height);

b8 vk_backend_begin_frame(render_backend_t *backend, float delta_time);
void vk_backend_update_global(mat4 projection, mat4 view, vec3 view_pos,
                              vec4 ambient_color, int32_t mode);
b8   vk_backend_end_frame(render_backend_t *backend, float delta_time);

#endif //__VULKAN_BACKEND_H__
