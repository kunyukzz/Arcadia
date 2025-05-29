#ifndef __VULKAN_BACKEND_H__
#define __VULKAN_BACKEND_H__

#include "engine/renderer/renderer_be.h"
#include "engine/resources/resc_type.h"

b8 vk_backend_init(render_backend_t *backend, const char *name);
void vk_backend_shut(render_backend_t *backend);
void vk_backend_resize(render_backend_t *backend, uint32_t width, uint32_t height);

b8 vk_backend_begin_frame(render_backend_t *backend, float delta_time);
void vk_backend_update_world(mat4 projection, mat4 view, vec3 view_pos,
                              vec4 ambient_color, int32_t mode);
void vk_backend_update_ui(mat4 projection, mat4 view, int32_t mode);
b8   vk_backend_end_frame(render_backend_t *backend, float delta_time);

b8 vk_backend_begin_renderpass(render_backend_t *be, uint8_t renderpass_id);
b8 vk_backend_end_renderpass(render_backend_t *be, uint8_t renderpass_id);

void vk_backend_geo_render(geo_render_data_t data);

void vk_backend_tex_init(const uint8_t *pixel, texture_t *texture);
void vk_backend_tex_shut(texture_t *texture);

b8 vk_backend_material_init(material_t *material);
void vk_backend_material_shut(material_t *material);

b8   vk_backend_geometry_init(geometry_t *geometry, uint32_t vertex_size,
                              uint32_t vertex_count, const void *vertices,
                              uint32_t idx_size, uint32_t idx_count,
                              const void *indices);

void vk_backend_geometry_shut(geometry_t *geometry);

#endif //__VULKAN_BACKEND_H__
