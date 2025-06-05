#ifndef __RENDERER_FRONTEND_H__
#define __RENDERER_FRONTEND_H__

#include "engine/define.h"
#include "engine/renderer/renderer_type.h"

struct shader_t;
struct shader_uniform_t;

b8 renderer_init(uint64_t *memory_require, void *state, const char *name);
void renderer_shut(void *state);
void renderer_resize(uint32_t width, uint32_t height);
b8 renderer_draw_frame(render_packet_t *packet);
void renderer_set_view(mat4 view);

void renderer_tex_init(const uint8_t *pixel, texture_t *texture);
void renderer_tex_shut(texture_t *texture);

b8   renderer_geometry_init(geometry_t *geometry, uint32_t vertex_size,
                            uint32_t vertex_count, const void *vertices,
                            uint32_t idx_size, uint32_t idx_count,
                            const void *indices);

void renderer_geometry_shut(geometry_t *geometry);

b8 renderer_renderpass_id(const char *name, uint8_t *renderpass_id);
b8   renderer_shader_create(struct shader_t *shader, uint8_t renderpass_id,
                                uint8_t stage_count, const char **stage_filenames,
                                shader_stage_t *stage);
void renderer_shader_shut(struct shader_t *shader);
b8 renderer_shader_init(struct shader_t *shader);
b8 renderer_shader_use(struct shader_t *shader);
b8 renderer_shader_bind_global(struct shader_t *shader);
b8 renderer_shader_bind_instance(struct shader_t *shader, uint32_t instance_id);
b8 renderer_shader_apply_global(struct shader_t *shader);
b8 renderer_shader_apply_instance(struct shader_t *shader);
b8 renderer_shader_acquire_inst_resc(struct shader_t *shader, uint32_t *instance_id);
b8 renderer_shader_release_inst_resc(struct shader_t *shader, uint32_t instance_id);
b8 renderer_set_uniform(struct shader_t *shader, struct shader_uniform_t *uniform, const void *value);

#endif //__RENDERER_FRONTEND_H__
