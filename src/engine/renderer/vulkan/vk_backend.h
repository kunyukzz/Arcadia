#ifndef __VULKAN_BACKEND_H__
#define __VULKAN_BACKEND_H__

#include "engine/renderer/renderer_be.h"
#include "engine/resources/resc_type.h"

b8 vk_backend_init(render_backend_t *backend, const char *name);
void vk_backend_shut(render_backend_t *backend);
void vk_backend_resize(render_backend_t *backend, uint32_t width, uint32_t height);

b8 vk_backend_begin_frame(render_backend_t *backend, float delta_time);
void vk_backend_update_global(mat4 projection, mat4 view, vec3 view_pos,
                              vec4 ambient_color, int32_t mode);
b8   vk_backend_end_frame(render_backend_t *backend, float delta_time);

void vk_backend_update_obj(geo_render_data_t data);

void vk_backend_tex_init(const char *name, int32_t width,
                         int32_t height, int32_t channel_count,
                         const uint8_t *pixel, b8 has_transparent,
                         texture_t *texture);
void vk_backend_tex_shut(texture_t *texture);

#endif //__VULKAN_BACKEND_H__
