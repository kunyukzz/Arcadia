#ifndef __VULKAN_BACKEND_H__
#define __VULKAN_BACKEND_H__

#include "engine/renderer/renderer_be.h"
#include "engine/resources/resc_type.h"

struct shader_t;
struct shader_uniform_t;

b8 vk_backend_init(render_backend_t *backend, const char *name);
void vk_backend_shut(render_backend_t *backend);
void vk_backend_resize(render_backend_t *backend, uint32_t width, uint32_t height);

b8 vk_backend_begin_frame(render_backend_t *backend, float delta_time);
b8   vk_backend_end_frame(render_backend_t *backend, float delta_time);

b8 vk_backend_begin_renderpass(render_backend_t *be, uint8_t renderpass_id);
b8 vk_backend_end_renderpass(render_backend_t *be, uint8_t renderpass_id);

void vk_backend_geo_render(geo_render_data_t data);

void vk_backend_tex_init(const uint8_t *pixel, texture_t *texture);
void vk_backend_tex_shut(texture_t *texture);

b8   vk_backend_geometry_init(geometry_t *geometry, uint32_t vertex_size,
                              uint32_t vertex_count, const void *vertices,
                              uint32_t idx_size, uint32_t idx_count,
                              const void *indices);

void vk_backend_geometry_shut(geometry_t *geometry);

b8 vk_backend_shader_create(struct shader_t *shader, uint8_t renderpass_id, uint8_t stage_count,
		const char **stage_filenames, shader_stage_t *stages);

void vk_backend_shader_shut(struct shader_t *shader);

b8 vk_backend_shader_init(struct shader_t* shader);
b8 vk_backend_shader_use(struct shader_t* shader);
b8 vk_backend_shader_bind_globals(struct shader_t* s);
b8 vk_backend_shader_bind_instance(struct shader_t* s, uint32_t instance_id);
b8 vk_backend_shader_apply_globals(struct shader_t* s);
b8 vk_backend_shader_apply_instance(struct shader_t* s);
b8 vk_backend_shader_acquire_inst_resc(struct shader_t* s, uint32_t* out_instance_id);
b8 vk_backend_shader_release_inst_resc(struct shader_t* s, uint32_t instance_id);
b8 vk_backend_set_uniform(struct shader_t* fe_shader, struct shader_uniform_t* uniform, const void* value);

#endif //__VULKAN_BACKEND_H__
